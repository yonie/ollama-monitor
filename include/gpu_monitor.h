#pragma once

#include <string>
#include <vector>

struct GPUInfo {
    bool available = false;
    int index = 0;
    std::string name;
    double total_vram_gb = 0.0;
    double used_vram_gb = 0.0;
    double free_vram_gb = 0.0;
    double utilization_percent = 0.0;
    int temperature_c = 0;
    int power_watts = 0;
    
    double getVRAMUsagePercent() const {
        if (total_vram_gb > 0) {
            return (used_vram_gb / total_vram_gb) * 100.0;
        }
        return 0.0;
    }
};

class GPUMonitor {
public:
    GPUMonitor();
    ~GPUMonitor();
    
    bool isAvailable() const;
    std::vector<GPUInfo> getGPUInfo();
    int getGPUCount() const;
    bool update();

private:
    bool initialized_;
    int gpu_count_;
    
    bool initializeNVML();
    void cleanupNVML();
};
