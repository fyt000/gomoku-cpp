# gomoku-cpp

A C++20 Gomoku engine with CLI, HTTP, desktop GUI, and MCP modes.
The HTTP server is compiled as C++23 because the current Glaze release requires it.

## Dependencies

Dependencies are declared in `vcpkg.json` and installed automatically by CMake's vcpkg toolchain:

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) for the HTTP server
- [Glaze](https://github.com/stephenberry/glaze) for typed JSON serialization and deserialization
- [raylib](https://www.raylib.com/) for the cross-platform desktop GUI

Install [vcpkg](https://github.com/microsoft/vcpkg), set `VCPKG_ROOT`, then configure and build with the supplied presets:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

The Visual Studio Debug executables are written to `out/build/windows-debug/Debug`.

## Test

```powershell
ctest --test-dir out/build/windows-debug -C Debug --output-on-failure
```

## Run

```powershell
.\out\build\windows-debug\Debug\gomoku-server.exe .\pattern.txt
```

The server listens on port 5000 and accepts JSON `POST` requests at:

- `/api/iswinner/` → `{ "winner": 0 | 1 | 2 }`
- `/api/getnextmove/` → `{ "x": number, "y": number }`

Each request body must contain a flattened 15×15 board: `{ "board": [0, ...] }`, where `0` is empty, `1` is black, and `2` is white.
Malformed request bodies are silently ignored with a `204 No Content` response.

The command-line demo is also available as `gomoku-cpp.exe` and takes the same pattern-file argument.

### Desktop GUI

```powershell
.\out\build\windows-debug\Debug\gomoku-gui.exe .\pattern.txt
```

The raylib GUI lets the player choose Black (first) or White (second), places black and white stones on a 15×15 grid, and retracts the last completed human/computer turn. It runs natively on Windows, macOS, and Linux (raylib's supported desktop platforms).

The side-panel effort selector controls search depth, frontier radius, and root-search threading:

- Light: depth 2, radius 2, one thread
- Medium: depth 3, radius 2, one thread
- High: depth 4, radius 2, one thread
- Max: depth 5, radius 3, up to four threads
- Ultra: depth 6, radius 3, up to eight threads

Thread counts are limited by the available hardware. The GUI also copies `pattern.txt` beside the executable, so it can be launched without an explicit pattern-file argument.

### MCP server

```powershell
.\out\build\windows-debug\Debug\gomoku-mcp.exe .\pattern.txt
```

`gomoku-mcp` is a stateful [Model Context Protocol](https://modelcontextprotocol.io/) server using the standard stdio transport: newline-delimited JSON-RPC is read from stdin and protocol responses are written only to stdout. Configure an MCP client to launch the command above; do not run it through a terminal that writes banners or other text to stdout.

It provides these tools:

- `new_game` — reset the session (Black moves first)
- `game_state` — read the flattened board and game status
- `play_move` — play the current side at zero-based `x` (row), `y` (column)
- `computer_move` — ask the built-in engine to play the current side

## VS Code

Install the recommended C++ and CMake Tools extensions when prompted, then open this folder in VS Code. Press `F5` and select **Debug Gomoku GUI**: VS Code configures the `windows-debug` preset, builds only `gomoku-gui`, launches it with `pattern.txt`, and attaches the Visual Studio C++ debugger. The build task locates either CMake on `PATH` or Visual Studio's bundled CMake.
