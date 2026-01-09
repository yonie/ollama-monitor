#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "../include/ollama_client.h"
#include "../include/gpu_monitor.h"
#include "../include/console_ui.h"

// Global flag for graceful shutdown
std::atomic<bool> g_running(true);

void signalHandler(int signum) {
    (void)signum;
    g_running = false;
}

void printUsage(const char* program_name) {
    std::cout << "Ollama Monitor - A top-like monitor for Ollama\n\n";
    std::cout << "Usage: " << program_name << " [OPTIONS]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -r, --refresh <sec>  Set refresh rate in seconds (default: 1)\n";
    std::cout << "  -u, --url <url>      Ollama server URL (default: http://localhost:11434)\n";
    std::cout << "  -1, --once           Run once and exit (for testing)\n";
    std::cout << "  -n, --count <num>    Run N times then exit\n";
    std::cout << "  --no-clear           Don't clear screen (for piped output)\n";
}

int main(int argc, char* argv[]) {
    // Default settings
    int refresh_rate = 1;
    std::string ollama_url = "http://localhost:11434";
    int run_count = 0;  // 0 = infinite
    bool no_clear = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if ((arg == "-r" || arg == "--refresh") && i + 1 < argc) {
            refresh_rate = std::stoi(argv[++i]);
            if (refresh_rate < 1) refresh_rate = 1;
        } else if ((arg == "-u" || arg == "--url") && i + 1 < argc) {
            ollama_url = argv[++i];
        } else if (arg == "-1" || arg == "--once") {
            run_count = 1;
        } else if ((arg == "-n" || arg == "--count") && i + 1 < argc) {
            run_count = std::stoi(argv[++i]);
            if (run_count < 1) run_count = 1;
        } else if (arg == "--no-clear") {
            no_clear = true;
        }
    }
    
    // Set up signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize components
    OllamaClient ollama_client(ollama_url);
    GPUMonitor gpu_monitor;
    ConsoleUI ui;
    
    ui.refreshRate(refresh_rate);
    ui.setNoClear(no_clear);
    
    // Initial connection check
    if (!ollama_client.isConnected()) {
        std::cerr << "\033[33mWarning: Cannot connect to Ollama server at " 
                  << ollama_url << "\033[0m\n";
        std::cerr << "Make sure Ollama is running. Will keep trying...\n";
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    // Main loop
    int iterations = 0;
    while (g_running) {
        DisplayInfo info;
        
        // Gather GPU information
        info.gpu_info = gpu_monitor.getGPUInfo();
        
        // Gather Ollama information
        info.ollama_status = ollama_client.getStatus();
        info.available_models = ollama_client.getModels();
        
        // Display
        ui.display(info);
        
        iterations++;
        
        // Check if we've hit the run count limit
        if (run_count > 0 && iterations >= run_count) {
            break;
        }
        
        // Wait for next refresh
        for (int i = 0; i < refresh_rate * 10 && g_running; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    // Clean exit
    if (run_count == 0) {
        std::cout << "\n\033[0mExiting...\n";
    }
    return 0;
}
