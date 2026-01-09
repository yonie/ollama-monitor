#pragma once

#include <string>

struct GPUInfo {
    bool available = false;
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
    GPUInfo getGPUInfo();
    bool update();

private:
    bool initialized_;
    
    bool initializeNVML();
    void cleanupNVML();
};
