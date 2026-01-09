#include "../include/console_ui.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#endif

ConsoleUI::ConsoleUI() : refresh_rate_(1), no_clear_(false) {
#ifdef _WIN32
    // Enable ANSI escape sequences on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    
    // Set console title
    SetConsoleTitleA("Ollama Monitor");
#endif
}

ConsoleUI::~ConsoleUI() {
}

void ConsoleUI::clearScreen() {
#ifdef _WIN32
    // Use ANSI escape codes for clearing
    std::cout << "\033[2J\033[H";
#else
    std::cout << "\033[2J\033[H";
#endif
    std::cout.flush();
}

std::string ConsoleUI::formatBytes(int64_t bytes) const {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    return oss.str();
}

std::string ConsoleUI::formatTimeUntil(const std::string& expires_at) const {
    if (expires_at.empty()) {
        return "N/A";
    }
    
    // Parse ISO8601 timestamp (simplified - handles basic format)
    // Format: 2024-01-15T10:30:00.123456Z
    std::tm tm = {};
    int year, month, day, hour, min, sec;
    
    if (sscanf(expires_at.c_str(), "%d-%d-%dT%d:%d:%d",
               &year, &month, &day, &hour, &min, &sec) == 6) {
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = min;
        tm.tm_sec = sec;
        
        // Convert to time_t (UTC)
        time_t expires_time = _mkgmtime(&tm);
        time_t now = time(nullptr);
        
        // Get current time in UTC
        std::tm* now_tm = gmtime(&now);
        time_t now_utc = _mkgmtime(now_tm);
        
        double diff_seconds = difftime(expires_time, now_utc);
        
        if (diff_seconds <= 0) {
            return "Expired";
        }
        
        int minutes = static_cast<int>(diff_seconds / 60);
        int seconds = static_cast<int>(diff_seconds) % 60;
        
        std::ostringstream oss;
        if (minutes > 0) {
            oss << minutes << "m " << seconds << "s";
        } else {
            oss << seconds << "s";
        }
        return oss.str();
    }
    
    return expires_at;
}

std::string ConsoleUI::truncateString(const std::string& str, size_t max_length) const {
    if (str.length() <= max_length) {
        return str;
    }
    return str.substr(0, max_length - 3) + "...";
}

std::string ConsoleUI::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&time_t_now);
    
    std::ostringstream oss;
    oss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string ConsoleUI::getProgressBar(double percentage, int width) const {
    int filled = static_cast<int>((percentage / 100.0) * width);
    if (filled > width) filled = width;
    if (filled < 0) filled = 0;
    
    std::string bar = "[";
    for (int i = 0; i < width; i++) {
        if (i < filled) {
            bar += "|";
        } else {
            bar += " ";
        }
    }
    bar += "]";
    return bar;
}

void ConsoleUI::displayGPUInfo(const GPUInfo& gpu_info) {
    std::cout << "\033[1;36m";  // Cyan bold
    std::cout << "=== GPU Status ===\033[0m\n";
    
    if (!gpu_info.available) {
        std::cout << "\033[33m  GPU monitoring unavailable (NVML not found)\033[0m\n";
        return;
    }
    
    // GPU Name
    std::cout << "  \033[1mGPU:\033[0m " << gpu_info.name << "\n";
    
    // VRAM Usage
    double vram_percent = gpu_info.getVRAMUsagePercent();
    std::cout << "  \033[1mVRAM:\033[0m ";
    
    // Color code based on usage
    if (vram_percent > 90) {
        std::cout << "\033[31m";  // Red
    } else if (vram_percent > 70) {
        std::cout << "\033[33m";  // Yellow
    } else {
        std::cout << "\033[32m";  // Green
    }
    
    std::cout << getProgressBar(vram_percent, 30) << " ";
    std::cout << std::fixed << std::setprecision(1) << vram_percent << "% ";
    std::cout << "(" << std::setprecision(2) << gpu_info.used_vram_gb << "/"
              << gpu_info.total_vram_gb << " GB)\033[0m\n";
    
    // GPU Utilization
    std::cout << "  \033[1mUtil:\033[0m ";
    if (gpu_info.utilization_percent > 90) {
        std::cout << "\033[31m";
    } else if (gpu_info.utilization_percent > 50) {
        std::cout << "\033[33m";
    } else {
        std::cout << "\033[32m";
    }
    std::cout << getProgressBar(gpu_info.utilization_percent, 30) << " "
              << std::fixed << std::setprecision(0) << gpu_info.utilization_percent << "%\033[0m\n";
    
    // Temperature & Power
    std::cout << "  \033[1mTemp:\033[0m ";
    if (gpu_info.temperature_c > 80) {
        std::cout << "\033[31m";
    } else if (gpu_info.temperature_c > 60) {
        std::cout << "\033[33m";
    } else {
        std::cout << "\033[32m";
    }
    std::cout << gpu_info.temperature_c << " C\033[0m  ";
    
    std::cout << "\033[1mPower:\033[0m " << gpu_info.power_watts << " W\n";
}

void ConsoleUI::displayRunningModels(const std::vector<OllamaRunningModel>& models) {
    std::cout << "\n\033[1;35m";  // Magenta bold
    std::cout << "=== Running Models ===\033[0m\n";
    
    if (models.empty()) {
        std::cout << "  \033[33mNo models currently loaded\033[0m\n";
        return;
    }
    
    // Header
    std::cout << "  \033[4m" << std::left
              << std::setw(30) << "MODEL"
              << std::setw(12) << "SIZE"
              << std::setw(12) << "PARAMS"
              << std::setw(10) << "QUANT"
              << std::setw(12) << "EXPIRES"
              << "\033[0m\n";
    
    for (const auto& model : models) {
        std::cout << "  \033[32m" << std::left
                  << std::setw(30) << truncateString(model.name, 29)
                  << "\033[0m"
                  << std::setw(12) << formatBytes(model.size)
                  << std::setw(12) << model.details.parameter_size
                  << std::setw(10) << model.details.quantization_level
                  << std::setw(12) << formatTimeUntil(model.expires_at)
                  << "\n";
    }
}

void ConsoleUI::displayAvailableModels(const std::vector<OllamaModel>& models) {
    std::cout << "\n\033[1;34m";  // Blue bold
    std::cout << "=== Available Models (" << models.size() << ") ===\033[0m\n";
    
    if (models.empty()) {
        std::cout << "  \033[33mNo models installed\033[0m\n";
        return;
    }
    
    // Show first 10 models
    size_t display_count = models.size() < 10 ? models.size() : 10;
    
    // Header
    std::cout << "  \033[4m" << std::left
              << std::setw(35) << "MODEL"
              << std::setw(12) << "SIZE"
              << "\033[0m\n";
    
    for (size_t i = 0; i < display_count; i++) {
        const auto& model = models[i];
        std::cout << "  " << std::left
                  << std::setw(35) << truncateString(model.name, 34)
                  << std::setw(12) << formatBytes(model.size)
                  << "\n";
    }
    
    if (models.size() > 10) {
        std::cout << "  \033[90m... and " << (models.size() - 10) << " more\033[0m\n";
    }
}

void ConsoleUI::displayOllamaInfo(const std::unique_ptr<OllamaStatus>& status) {
    if (!status) {
        std::cout << "\n\033[1;31m";
        std::cout << "=== Ollama Status ===\033[0m\n";
        std::cout << "  \033[31mCannot connect to Ollama server\033[0m\n";
        std::cout << "  \033[90mMake sure Ollama is running (ollama serve)\033[0m\n";
        return;
    }
    
    displayRunningModels(status->models);
}

void ConsoleUI::display(const DisplayInfo& info) {
    if (!no_clear_) {
        clearScreen();
    }
    
    // Header
    std::cout << "\033[1;37;44m";  // White on blue
    std::cout << " OLLAMA MONITOR                                              ";
    std::cout << getCurrentTime() << " \033[0m\n\n";
    
    // GPU Information
    displayGPUInfo(info.gpu_info);
    
    // Ollama Status
    displayOllamaInfo(info.ollama_status);
    
    // Available Models
    displayAvailableModels(info.available_models);
    
    // Footer
    std::cout << "\n\033[90mPress Ctrl+C to exit | Refreshing every " 
              << refresh_rate_ << "s\033[0m\n";
}
