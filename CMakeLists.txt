cmake_minimum_required(VERSION 3.20)
project(ollama-monitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/ollama_client.cpp
    src/gpu_monitor.cpp
    src/console_ui.cpp
)

# Header files
set(HEADERS
    include/ollama_client.h
    include/gpu_monitor.h
    include/console_ui.h
)

# Create executable
add_executable(ollama-monitor ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(ollama-monitor PRIVATE include)

# Windows specific settings
if(WIN32)
    # WinHTTP for HTTP requests, DXGI for GPU info
    target_link_libraries(ollama-monitor winhttp dxgi)
endif()

# Set compiler-specific options
if(MSVC)
    target_compile_options(ollama-monitor PRIVATE /W4)
    target_compile_definitions(ollama-monitor PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(ollama-monitor PRIVATE -Wall -Wextra -Wpedantic)
endif()
