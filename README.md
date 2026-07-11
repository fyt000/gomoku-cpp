# gomoku-cpp

A C++20 Gomoku engine with a small HTTP API.
The HTTP server is compiled as C++23 because the current Glaze release requires it.

## Dependencies

Dependencies are declared in `vcpkg.json` and installed automatically by CMake's vcpkg toolchain:

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) for the HTTP server
- [Glaze](https://github.com/stephenberry/glaze) for typed JSON serialization and deserialization

Install [vcpkg](https://github.com/microsoft/vcpkg), set `VCPKG_ROOT`, then configure and build with the supplied presets:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
```

The executables are written to `out/build/windows-debug`.

## Test

```powershell
ctest --test-dir out/build/windows-debug -C Debug --output-on-failure
```

## Run

```powershell
.\out\build\windows-debug\gomoku-server.exe .\pattern.txt
```

The server listens on port 5000 and accepts JSON `POST` requests at:

- `/api/iswinner/` → `{ "winner": 0 | 1 | 2 }`
- `/api/getnextmove/` → `{ "x": number, "y": number }`

Each request body must contain a flattened 15×15 board: `{ "board": [0, ...] }`, where `0` is empty, `1` is black, and `2` is white.
Malformed request bodies are silently ignored with a `204 No Content` response.

The command-line demo is also available as `gomoku-cpp.exe` and takes the same pattern-file argument.
