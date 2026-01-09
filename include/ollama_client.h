#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>

struct OllamaModel {
    std::string name;
    std::string model;
    int64_t size;
    std::string digest;
    std::string modified_at;
};

struct OllamaRunningModel {
    std::string name;
    std::string model;
    int64_t size;
    std::string expires_at;
    std::string digest;
    
    struct {
        std::string parent_model;
        std::string format;
        std::string family;
        std::vector<std::string> families;
        std::string parameter_size;
        std::string quantization_level;
    } details;
};

struct OllamaStatus {
    std::vector<OllamaRunningModel> models;
};

class OllamaClient {
public:
    OllamaClient(const std::string& base_url = "http://localhost:11434");
    ~OllamaClient();

    bool isConnected() const;
    
    std::unique_ptr<OllamaStatus> getStatus();
    std::vector<OllamaModel> getModels();

private:
    std::string base_url_;
    bool connected_;
    
    bool testConnection();
    std::string makeRequest(const std::string& endpoint);
    std::vector<std::string> split(const std::string& s, char delimiter);
    std::string trim(const std::string& str);
    
    // Simple JSON parsing (since we want minimal dependencies)
    OllamaRunningModel parseRunningModel(const std::string& json_str);
    OllamaModel parseModel(const std::string& json_str);
    std::string extractStringValue(const std::string& json, const std::string& key);
    int64_t extractIntValue(const std::string& json, const std::string& key);
    std::vector<std::string> extractArrayValues(const std::string& json, const std::string& key);
};