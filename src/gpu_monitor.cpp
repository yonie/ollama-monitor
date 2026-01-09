#include "../include/gpu_monitor.h"
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")

// NVML types for dynamic loading
typedef int nvmlReturn_t;
typedef struct nvmlDevice_st* nvmlDevice_t;
typedef struct { unsigned long long total; unsigned long long free; unsigned long long used; } nvmlMemory_t;
typedef struct { unsigned int gpu; unsigned int memory; } nvmlUtilization_t;
typedef enum { NVML_TEMPERATURE_GPU = 0 } nvmlTemperatureSensors_t;

#define NVML_SUCCESS 0
#define NVML_DEVICE_NAME_BUFFER_SIZE 64

// Function pointer types
typedef nvmlReturn_t (*nvmlInit_t)(void);
typedef nvmlReturn_t (*nvmlShutdown_t)(void);
typedef nvmlReturn_t (*nvmlDeviceGetCount_t)(unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t (*nvmlDeviceGetName_t)(nvmlDevice_t, char*, unsigned int);
typedef nvmlReturn_t (*nvmlDeviceGetMemoryInfo_t)(nvmlDevice_t, nvmlMemory_t*);
typedef nvmlReturn_t (*nvmlDeviceGetUtilizationRates_t)(nvmlDevice_t, nvmlUtilization_t*);
typedef nvmlReturn_t (*nvmlDeviceGetTemperature_t)(nvmlDevice_t, nvmlTemperatureSensors_t, unsigned int*);
typedef nvmlReturn_t (*nvmlDeviceGetPowerUsage_t)(nvmlDevice_t, unsigned int*);

// Global NVML state
static HMODULE g_nvmlDll = nullptr;
static nvmlInit_t g_nvmlInit = nullptr;
static nvmlShutdown_t g_nvmlShutdown = nullptr;
static nvmlDeviceGetCount_t g_nvmlDeviceGetCount = nullptr;
static nvmlDeviceGetHandleByIndex_t g_nvmlDeviceGetHandleByIndex = nullptr;
static nvmlDeviceGetName_t g_nvmlDeviceGetName = nullptr;
static nvmlDeviceGetMemoryInfo_t g_nvmlDeviceGetMemoryInfo = nullptr;
static nvmlDeviceGetUtilizationRates_t g_nvmlDeviceGetUtilizationRates = nullptr;
static nvmlDeviceGetTemperature_t g_nvmlDeviceGetTemperature = nullptr;
static nvmlDeviceGetPowerUsage_t g_nvmlDeviceGetPowerUsage = nullptr;

static bool loadNvmlFunctions() {
    // Try to load nvml.dll from system (comes with NVIDIA driver)
    g_nvmlDll = LoadLibraryA("nvml.dll");
    if (!g_nvmlDll) {
        // Try alternate location
        g_nvmlDll = LoadLibraryA("C:\\Program Files\\NVIDIA Corporation\\NVSMI\\nvml.dll");
    }
    if (!g_nvmlDll) {
        return false;
    }
    
    g_nvmlInit = (nvmlInit_t)GetProcAddress(g_nvmlDll, "nvmlInit_v2");
    if (!g_nvmlInit) g_nvmlInit = (nvmlInit_t)GetProcAddress(g_nvmlDll, "nvmlInit");
    
    g_nvmlShutdown = (nvmlShutdown_t)GetProcAddress(g_nvmlDll, "nvmlShutdown");
    g_nvmlDeviceGetCount = (nvmlDeviceGetCount_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetCount_v2");
    if (!g_nvmlDeviceGetCount) g_nvmlDeviceGetCount = (nvmlDeviceGetCount_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetCount");
    g_nvmlDeviceGetHandleByIndex = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetHandleByIndex");
    g_nvmlDeviceGetName = (nvmlDeviceGetName_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetName");
    g_nvmlDeviceGetMemoryInfo = (nvmlDeviceGetMemoryInfo_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetMemoryInfo");
    g_nvmlDeviceGetUtilizationRates = (nvmlDeviceGetUtilizationRates_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetUtilizationRates");
    g_nvmlDeviceGetTemperature = (nvmlDeviceGetTemperature_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetTemperature");
    g_nvmlDeviceGetPowerUsage = (nvmlDeviceGetPowerUsage_t)GetProcAddress(g_nvmlDll, "nvmlDeviceGetPowerUsage");
    
    if (!g_nvmlInit || !g_nvmlShutdown || !g_nvmlDeviceGetHandleByIndex || !g_nvmlDeviceGetCount) {
        FreeLibrary(g_nvmlDll);
        g_nvmlDll = nullptr;
        return false;
    }
    
    return true;
}

#endif // _WIN32

GPUMonitor::GPUMonitor() : initialized_(false), gpu_count_(0) {
#ifdef _WIN32
    initialized_ = initializeNVML();
#endif
}

GPUMonitor::~GPUMonitor() {
#ifdef _WIN32
    cleanupNVML();
#endif
}

bool GPUMonitor::isAvailable() const {
    return initialized_;
}

bool GPUMonitor::initializeNVML() {
#ifdef _WIN32
    if (!loadNvmlFunctions()) {
        return false;
    }
    
    nvmlReturn_t result = g_nvmlInit();
    if (result != NVML_SUCCESS) {
        FreeLibrary(g_nvmlDll);
        g_nvmlDll = nullptr;
        return false;
    }
    
    // Get the number of GPUs
    unsigned int deviceCount = 0;
    result = g_nvmlDeviceGetCount(&deviceCount);
    if (result != NVML_SUCCESS || deviceCount == 0) {
        g_nvmlShutdown();
        FreeLibrary(g_nvmlDll);
        g_nvmlDll = nullptr;
        return false;
    }
    
    gpu_count_ = static_cast<int>(deviceCount);
    return true;
#else
    return false;
#endif
}

int GPUMonitor::getGPUCount() const {
    return gpu_count_;
}

void GPUMonitor::cleanupNVML() {
#ifdef _WIN32
    if (g_nvmlDll) {
        if (g_nvmlShutdown) {
            g_nvmlShutdown();
        }
        FreeLibrary(g_nvmlDll);
        g_nvmlDll = nullptr;
    }
    initialized_ = false;
#endif
}

bool GPUMonitor::update() {
    return initialized_;
}

std::vector<GPUInfo> GPUMonitor::getGPUInfo() {
    std::vector<GPUInfo> infos;
    
#ifdef _WIN32
    if (!initialized_ || !g_nvmlDll) {
        // Try DXGI fallback for basic info - enumerate all adapters
        IDXGIFactory* pFactory = nullptr;
        if (SUCCEEDED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory))) {
            IDXGIAdapter* pAdapter = nullptr;
            for (UINT i = 0; pFactory->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; i++) {
                DXGI_ADAPTER_DESC desc;
                if (SUCCEEDED(pAdapter->GetDesc(&desc))) {
                    // Skip software adapters (like Microsoft Basic Render Driver)
                    if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c) {
                        pAdapter->Release();
                        continue;
                    }
                    
                    GPUInfo info;
                    info.available = true;
                    info.index = static_cast<int>(i);
                    
                    // Convert wide string to narrow
                    char name[128];
                    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), NULL, NULL);
                    info.name = name;
                    
                    // DXGI gives dedicated video memory
                    info.total_vram_gb = static_cast<double>(desc.DedicatedVideoMemory) / (1024.0 * 1024.0 * 1024.0);
                    // Note: DXGI doesn't give current usage, only total
                    info.used_vram_gb = 0;
                    info.free_vram_gb = info.total_vram_gb;
                    
                    infos.push_back(info);
                }
                pAdapter->Release();
            }
            pFactory->Release();
        }
        return infos;
    }
    
    // NVML path - enumerate all GPUs
    for (int i = 0; i < gpu_count_; i++) {
        GPUInfo info;
        info.index = i;
        
        nvmlDevice_t device;
        if (g_nvmlDeviceGetHandleByIndex(static_cast<unsigned int>(i), &device) != NVML_SUCCESS) {
            continue;
        }
        
        info.available = true;
        
        // Get GPU name
        if (g_nvmlDeviceGetName) {
            char name[NVML_DEVICE_NAME_BUFFER_SIZE];
            if (g_nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE) == NVML_SUCCESS) {
                info.name = name;
            }
        }
        
        // Get memory info
        if (g_nvmlDeviceGetMemoryInfo) {
            nvmlMemory_t memory;
            if (g_nvmlDeviceGetMemoryInfo(device, &memory) == NVML_SUCCESS) {
                info.total_vram_gb = static_cast<double>(memory.total) / (1024.0 * 1024.0 * 1024.0);
                info.used_vram_gb = static_cast<double>(memory.used) / (1024.0 * 1024.0 * 1024.0);
                info.free_vram_gb = static_cast<double>(memory.free) / (1024.0 * 1024.0 * 1024.0);
            }
        }
        
        // Get GPU utilization
        if (g_nvmlDeviceGetUtilizationRates) {
            nvmlUtilization_t utilization;
            if (g_nvmlDeviceGetUtilizationRates(device, &utilization) == NVML_SUCCESS) {
                info.utilization_percent = static_cast<double>(utilization.gpu);
            }
        }
        
        // Get temperature
        if (g_nvmlDeviceGetTemperature) {
            unsigned int temp;
            if (g_nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
                info.temperature_c = static_cast<int>(temp);
            }
        }
        
        // Get power usage
        if (g_nvmlDeviceGetPowerUsage) {
            unsigned int power;
            if (g_nvmlDeviceGetPowerUsage(device, &power) == NVML_SUCCESS) {
                info.power_watts = static_cast<int>(power / 1000); // Convert from milliwatts
            }
        }
        
        infos.push_back(info);
    }
#endif
    
    return infos;
}
