# Ollama Monitor

A lightweight, console-based monitoring tool for [Ollama](https://ollama.ai/) on Windows. Inspired by Linux `top`, it provides real-time visibility into your local LLM inference server.

![Screenshot placeholder]

## Features

- **GPU Monitoring**: Real-time VRAM usage, GPU utilization, temperature, and power consumption (full metrics for NVIDIA; basic support for AMD/Intel)
- **Ollama Integration**: View running models, loaded context, and available models
- **Lightweight**: Native Windows application with no external dependencies
- **Top-style UI**: Clean, color-coded console interface with auto-refresh

## Requirements

- Windows 10/11
- NVIDIA GPU recommended (for full GPU monitoring); AMD/Intel GPUs supported with basic info via DXGI
- [Ollama](https://ollama.ai/) installed and running
- Visual Studio 2022 Build Tools or Visual Studio 2022 (for building)
- CMake 3.20+

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/ollama-monitor.git
cd ollama-monitor

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release
```

The executable will be at `build/Release/ollama-monitor.exe`.

## Usage

```bash
# Run with default settings (1 second refresh)
ollama-monitor

# Custom refresh rate (2 seconds)
ollama-monitor -r 2

# Connect to Ollama on a different host/port
ollama-monitor -u http://192.168.1.100:11434

# Run once and exit (useful for scripts)
ollama-monitor --once

# Run once without clearing screen (useful for capturing output)
ollama-monitor --once --no-clear
```

### Command Line Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-r, --refresh <sec>` | Set refresh rate in seconds (default: 1) |
| `-u, --url <url>` | Ollama server URL (default: http://localhost:11434) |
| `-1, --once` | Run once and exit |
| `-n, --count <num>` | Run N times then exit |
| `--no-clear` | Don't clear screen between updates |

### Keyboard Controls

- `Ctrl+C` - Exit

## Output

```
 OLLAMA MONITOR                                              2026-01-09 19:36:07

=== GPU Status ===
  GPU 0: NVIDIA GeForce RTX 5090
  VRAM: [||||||                        ] 25.3% (4.03/15.93 GB)
  Util: [||||||||||||                  ] 42%
  Temp: 62 C  Power: 145 W

  GPU 1: NVIDIA GeForce RTX 4080
  VRAM: [|||                           ] 12.1% (1.94/16.00 GB)
  Util: [||                            ] 8%
  Temp: 45 C  Power: 35 W

=== Running Models ===
  MODEL                         SIZE        PARAMS      QUANT     EXPIRES
  llama3:8b                     4.7 GB      8B          Q4_K_M    4m 32s

=== Available Models (5) ===
  MODEL                              SIZE
  devstral-small-2:latest            14.1 GB
  nomic-embed-text-v2-moe:latest     913.3 MB
  ministral-3:14b-instruct-2512-q... 8.5 GB
  qwen3-vl:30b-a3b-instruct-q4_K_M   18.2 GB
  nomic-embed-text:latest            261.6 MB

Press Ctrl+C to exit | Refreshing every 1s
```

## How It Works

### GPU Monitoring

The application supports multiple GPUs and will display all detected graphics cards.

#### NVIDIA GPUs (Full Support)

Dynamically loads `nvml.dll` from the NVIDIA driver (no CUDA toolkit required). This provides:
- GPU name and model
- VRAM total/used/free
- GPU utilization percentage
- Temperature
- Power consumption

#### AMD / Intel / Other GPUs (Basic Support)

Falls back to DXGI which provides:
- GPU name and model
- Total VRAM

**Not available via DXGI:**
- Current VRAM usage (shows 0%)
- GPU utilization
- Temperature
- Power consumption

This is a limitation of the DXGI API itself. Full support for AMD GPUs would require integrating AMD's ADL (AMD Display Library) SDK, and Intel GPUs would need Intel's IGCL or Level Zero API. Contributions welcome!

### Ollama Integration

Uses Ollama's REST API:
- `/api/tags` - List available models
- `/api/ps` - List running/loaded models

HTTP requests use Windows native WinHTTP - no external dependencies like curl.

## Project Structure

```
ollama-monitor/
├── CMakeLists.txt           # Build configuration
├── README.md                # This file
├── LICENSE                  # MIT License
├── include/
│   ├── ollama_client.h      # Ollama API client
│   ├── gpu_monitor.h        # GPU monitoring
│   └── console_ui.h         # Console UI
└── src/
    ├── main.cpp             # Entry point
    ├── ollama_client.cpp    # HTTP client (WinHTTP)
    ├── gpu_monitor.cpp      # NVML/DXGI GPU monitoring
    └── console_ui.cpp       # Top-style display
```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- [Ollama](https://ollama.ai/) for the excellent local LLM runtime
- NVIDIA for NVML

## Related Projects

- [nvtop](https://github.com/Syllo/nvtop) - Linux GPU monitoring
- [btop](https://github.com/aristocratos/btop) - Linux system monitor
