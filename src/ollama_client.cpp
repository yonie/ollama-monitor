#include "../include/ollama_client.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif

OllamaClient::OllamaClient(const std::string& base_url) 
    : base_url_(base_url), connected_(false) {
    connected_ = testConnection();
}

OllamaClient::~OllamaClient() {
}

bool OllamaClient::isConnected() const {
    return connected_;
}

bool OllamaClient::testConnection() {
    try {
        std::string response = makeRequest("/api/tags");
        bool ok = !response.empty() && response.find("\"models\"") != std::string::npos;
        return ok;
    } catch (...) {
        return false;
    }
}

std::string OllamaClient::makeRequest(const std::string& endpoint) {
#ifdef _WIN32
    std::string result;
    
    // Parse the base URL to extract host and port
    std::string host = "localhost";
    int port = 11434;
    
    // Simple URL parsing for http://host:port format
    std::string url = base_url_;
    if (url.find("http://") == 0) {
        url = url.substr(7);
    } else if (url.find("https://") == 0) {
        url = url.substr(8);
    }
    
    size_t colon_pos = url.find(':');
    if (colon_pos != std::string::npos) {
        host = url.substr(0, colon_pos);
        std::string port_str = url.substr(colon_pos + 1);
        // Remove trailing slash if present
        size_t slash_pos = port_str.find('/');
        if (slash_pos != std::string::npos) {
            port_str = port_str.substr(0, slash_pos);
        }
        try {
            port = std::stoi(port_str);
        } catch (...) {
            port = 11434;
        }
    } else {
        size_t slash_pos = url.find('/');
        if (slash_pos != std::string::npos) {
            host = url.substr(0, slash_pos);
        } else {
            host = url;
        }
    }
    
    // Convert host to wide string
    std::wstring whost(host.begin(), host.end());
    std::wstring wendpoint(endpoint.begin(), endpoint.end());
    
    // Initialize WinHTTP
    HINTERNET hSession = WinHttpOpen(L"OllamaMonitor/1.0",
                                      WINHTTP_ACCESS_TYPE_NO_PROXY,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        return "";
    }
    
    // Set timeouts (5 seconds)
    WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);
    
    // Connect to server
    HINTERNET hConnect = WinHttpConnect(hSession, whost.c_str(), 
                                         static_cast<INTERNET_PORT>(port), 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Create request
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", wendpoint.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return "";
    }
    
    // Send request
    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                       WINHTTP_NO_REQUEST_DATA, 0,
                                       0, 0);
    
    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    }
    
    // Read response
    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }
            
            if (dwSize == 0) {
                break;
            }
            
            char* pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer) {
                break;
            }
            
            ZeroMemory(pszOutBuffer, dwSize + 1);
            
            if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                result.append(pszOutBuffer, dwDownloaded);
            }
            
            delete[] pszOutBuffer;
        } while (dwSize > 0);
    }
    
    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    
    return result;
#else
    return "";
#endif
}

std::vector<std::string> OllamaClient::split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string OllamaClient::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

std::string OllamaClient::extractStringValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) {
        search_key = "\"" + key + "\": \"";
        pos = json.find(search_key);
    }
    
    if (pos != std::string::npos) {
        pos += search_key.length();
        size_t end = json.find("\"", pos);
        if (end != std::string::npos) {
            return json.substr(pos, end - pos);
        }
    }
    return "";
}

int64_t OllamaClient::extractIntValue(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\":";
    size_t pos = json.find(search_key);
    if (pos != std::string::npos) {
        pos += search_key.length();
        // Skip whitespace
        while (pos < json.length() && std::isspace(json[pos])) {
            pos++;
        }
        size_t end = json.find_first_of(",}", pos);
        if (end != std::string::npos) {
            std::string num_str = trim(json.substr(pos, end - pos));
            try {
                return std::stoll(num_str);
            } catch (...) {
                return 0;
            }
        }
    }
    return 0;
}

std::vector<std::string> OllamaClient::extractArrayValues(const std::string& json, const std::string& key) {
    std::vector<std::string> values;
    std::string search_key = "\"" + key + "\":[";
    size_t pos = json.find(search_key);
    if (pos != std::string::npos) {
        pos += search_key.length();
        size_t end = json.find("]", pos);
        if (end != std::string::npos) {
            std::string array_str = json.substr(pos, end - pos);
            auto items = split(array_str, ',');
            for (auto& item : items) {
                item = trim(item);
                if (item.length() >= 2 && item.front() == '"' && item.back() == '"') {
                    values.push_back(item.substr(1, item.length() - 2));
                }
            }
        }
    }
    return values;
}

OllamaRunningModel OllamaClient::parseRunningModel(const std::string& json_str) {
    OllamaRunningModel model;
    
    model.name = extractStringValue(json_str, "name");
    model.model = extractStringValue(json_str, "model");
    model.size = extractIntValue(json_str, "size");
    model.expires_at = extractStringValue(json_str, "expires_at");
    model.digest = extractStringValue(json_str, "digest");
    
    // Parse details object
    model.details.parent_model = extractStringValue(json_str, "parent_model");
    model.details.format = extractStringValue(json_str, "format");
    model.details.family = extractStringValue(json_str, "family");
    model.details.parameter_size = extractStringValue(json_str, "parameter_size");
    model.details.quantization_level = extractStringValue(json_str, "quantization_level");
    model.details.families = extractArrayValues(json_str, "families");
    
    return model;
}

OllamaModel OllamaClient::parseModel(const std::string& json_str) {
    OllamaModel model;
    
    model.name = extractStringValue(json_str, "name");
    model.model = extractStringValue(json_str, "model");
    model.size = extractIntValue(json_str, "size");
    model.digest = extractStringValue(json_str, "digest");
    model.modified_at = extractStringValue(json_str, "modified_at");
    
    return model;
}

std::unique_ptr<OllamaStatus> OllamaClient::getStatus() {
    std::string response = makeRequest("/api/ps");
    if (response.empty()) {
        return nullptr;
    }

    auto status = std::make_unique<OllamaStatus>();
    
    // Parse models array - need to handle nested brackets
    std::string models_key = "\"models\":[";
    size_t pos = response.find(models_key);
    if (pos != std::string::npos) {
        pos += models_key.length();
        
        // Find the matching closing bracket for the models array
        int bracket_count = 1;
        size_t end = pos;
        for (; end < response.length() && bracket_count > 0; ++end) {
            if (response[end] == '[') bracket_count++;
            else if (response[end] == ']') bracket_count--;
        }
        
        if (bracket_count == 0) {
            std::string models_array = response.substr(pos, end - pos - 1);
            
            // Parse individual model objects
            int brace_count = 0;
            size_t start = 0;
            for (size_t i = 0; i < models_array.length(); ++i) {
                if (models_array[i] == '{') {
                    if (brace_count == 0) {
                        start = i;
                    }
                    brace_count++;
                } else if (models_array[i] == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        std::string model_json = models_array.substr(start, i - start + 1);
                        auto model = parseRunningModel(model_json);
                        if (!model.name.empty()) {
                            status->models.push_back(model);
                        }
                    }
                }
            }
        }
    }
    
    return status;
}

std::vector<OllamaModel> OllamaClient::getModels() {
    std::string response = makeRequest("/api/tags");
    if (response.empty()) {
        return {};
    }

    std::vector<OllamaModel> models;
    
    // Parse models array - need to handle nested brackets
    std::string models_key = "\"models\":[";
    size_t pos = response.find(models_key);
    if (pos != std::string::npos) {
        pos += models_key.length();
        
        // Find the matching closing bracket for the models array
        int bracket_count = 1;
        size_t end = pos;
        for (; end < response.length() && bracket_count > 0; ++end) {
            if (response[end] == '[') bracket_count++;
            else if (response[end] == ']') bracket_count--;
        }
        
        if (bracket_count == 0) {
            std::string models_array = response.substr(pos, end - pos - 1);
            
            // Parse individual model objects
            int brace_count = 0;
            size_t start = 0;
            for (size_t i = 0; i < models_array.length(); ++i) {
                if (models_array[i] == '{') {
                    if (brace_count == 0) {
                        start = i;
                    }
                    brace_count++;
                } else if (models_array[i] == '}') {
                    brace_count--;
                    if (brace_count == 0) {
                        std::string model_json = models_array.substr(start, i - start + 1);
                        auto model = parseModel(model_json);
                        if (!model.name.empty()) {
                            models.push_back(model);
                        }
                    }
                }
            }
        }
    }
    
    return models;
}
