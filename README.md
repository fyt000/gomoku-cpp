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
.\out\build\windows-debug\Debug\gomoku-server.exe
```

The server listens on port 5000 and accepts JSON `POST` requests at:

- `/api/iswinner/` → `{ "winner": 0 | 1 | 2 }`
- `/api/getnextmove/` → `{ "x": number, "y": number }`

Each request body must contain a flattened 15×15 board: `{ "board": [0, ...] }`, where `0` is empty, `1` is black, and `2` is white.
Malformed request bodies are silently ignored with a `204 No Content` response.

The command-line demo is also available as `gomoku-cpp.exe`. All executables use
the evaluation cache compiled from `pattern.txt` by default. An explicit pattern
file can still be supplied as the sole argument for tuning and comparison.

Every executable can print the project license and all bundled third-party
notices without opening the GUI or starting a server:

```powershell
.\out\build\windows-debug\Debug\gomoku-gui.exe --licenses
```

Each non-comment line in `pattern.txt` has the form
`pattern opponent-to-move owner-to-move`. The first score applies when the other
player moves next; the second applies when the player owning the pattern moves
next. This lets forcing shapes such as half-open fours carry different values
depending on whose turn it is.

CMake regenerates constexpr pattern definitions whenever `pattern.txt` changes.
The compiler then builds the complete 65,536-row cache into the engine, avoiding
file I/O and dynamic-programming initialization during normal startup.

### Desktop GUI

```powershell
.\out\build\windows-debug\Debug\gomoku-gui.exe
```

The raylib GUI lets the player choose Black (first) or White (second), places black and white stones on a 15×15 grid, and retracts the last completed human/computer turn. It runs natively on Windows, macOS, and Linux (raylib's supported desktop platforms).

When multiple moves have the same best evaluation, the engine chooses between
them using a random seed. The active seed is shown in the GUI. Click the field
to edit it, then press Enter or **Apply**; **Random** generates a fresh seed and
**Copy** puts the active seed on the clipboard. Pass a seed to reproduce a game
while debugging; without one, a seed is generated at startup:

```powershell
.\out\build\windows-debug\Debug\gomoku-gui.exe --seed 12345
.\out\build\windows-debug\Debug\gomoku-gui.exe --seed 12345 pattern.txt
```

The side-panel effort selector controls search depth, frontier radius, and root-search threading:

- Light: depth 2, radius 1, one thread
- Medium: depth 3, radius 2, one thread
- High: depth 5, radius 2, one thread
- Max: depth 6, radius 2, up to four threads
- Ultra: depth 6, radius 3, up to eight threads

Thread counts are limited by the available physical CPU cores on Windows (so
Hyper-Threading siblings do not count) and by the logical CPU count reported by
standard C++ on other platforms.

### MCP server

```powershell
.\out\build\windows-debug\Debug\gomoku-mcp.exe
```

`gomoku-mcp` is a stateful [Model Context Protocol](https://modelcontextprotocol.io/) server using the standard stdio transport: newline-delimited JSON-RPC is read from stdin and protocol responses are written only to stdout. Configure an MCP client to launch the command above; do not run it through a terminal that writes banners or other text to stdout.

It provides these tools:

- `new_game` — reset the session (Black moves first)
- `game_state` — read the flattened board and game status
- `play_move` — play the current side at zero-based `x` (row), `y` (column)
- `computer_move` — ask the built-in engine to play the current side; accepts an optional
  `effort` of `light`, `medium`, `high` (default), `max`, or `ultra`, matching the GUI presets

Game-state responses include `boardSize: 15` and a 15×15 nested `board`, indexed as
`board[x][y]`, where `0` is empty, `1` is Black, and `2` is White.

## VS Code

Install the recommended C++ and CMake Tools extensions when prompted, then open this folder in VS Code. Press `F5` and select **Debug Gomoku GUI**: VS Code configures the `windows-debug` preset, builds only `gomoku-gui`, launches it with `pattern.txt` as a runtime override, and attaches the Visual Studio C++ debugger. The build task locates either CMake on `PATH` or Visual Studio's bundled CMake.

## License

gomoku-cpp is open source under the [MIT License](LICENSE). Dependencies retain
their own permissive licenses; their copyright statements and license terms are
collected in [THIRD_PARTY_NOTICES.txt](THIRD_PARTY_NOTICES.txt) and embedded in
each executable for display with `--licenses`.
