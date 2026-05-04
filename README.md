# Chatbattles

Desktop client built with [raylib](https://www.raylib.com/) and [Dear ImGui](https://github.com/ocornut/imgui) (docking branch) via [rlImGui](https://github.com/raylib-extras/rlImGui). It connects to a WebSocket server, spawns balls for chat users from JSON messages, and handles simple chat commands and gift events.

## Features

- Resizable window, ImGui docking layout, debug-style config panel
- WebSocket client ([IXWebSocket](https://github.com/machinezone/IXWebSocket)) with TLS enabled in the build
- JSON parsing ([nlohmann/json](https://github.com/nlohmann/json))
- Vendored third-party code under `external/` (no git submodules in this repository)

## Requirements

Build tooling and libraries expected by the current `CMakeLists.txt`:

- CMake 3.10 or newer (your vendored raylib may require a newer CMake; if configure fails, upgrade CMake to match raylib’s stated minimum)
- A C++17 compiler
- Development packages for **OpenSSL**, **libcurl**, and **libwebp**, plus normal graphics and pthread support on your platform (exact package names depend on the distribution)

On Windows, the project links `winmm` and `gdi32` in addition to the above.

## Configure and build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable target is named **RaylibEngine**. With a single-configuration generator, the binary is typically at `build/RaylibEngine` (exact path may vary).

## Run

By default the client opens a window titled **ChatBattle** and connects to:

`ws://127.0.0.1:8080`

Change the URL in `src/core/application.cpp` if your server listens elsewhere. For `wss://` endpoints you need a TLS-enabled server and a build where IXWebSocket TLS and OpenSSL are correctly configured.

## Repository layout

| Path | Purpose |
|------|---------|
| `src/` | Application entry point and implementation |
| `include/` | Public headers grouped by area (`core/`, `gamecore/`, `network/`, `tools/`) |
| `external/` | Vendored raylib, ImGui, rlImGui, IXWebSocket, nlohmann/json |
| `web/Service/` | Separate web front-end (Vite + React); see that folder if you work on the web stack |

## License

Third-party libraries under `external/` keep their respective licenses. Add or link a top-level license for your own game code if you distribute binaries.
