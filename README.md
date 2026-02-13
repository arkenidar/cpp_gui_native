# GUI C++

Mobile-first native UI MVP: iterate fast on desktop (SDL + Lua hot-reload) and carry the same C++ core toward Android/NDK.

## Project Vision
- Deliver a mobile-first app architecture where core runtime and rendering stay native (C++), while UI iteration remains fast and script-driven.
- Keep Android/NDK compatibility as a first-class target, using desktop as the primary daily development host for speed.
- Enable developers to prototype and validate features quickly on desktop, then carry the same app model to mobile with minimal rewrite.
- Reduce time from concept to production-ready native UX by combining a tight local dev loop with portable core systems.

## Current scope
- Desktop host using SDL3 (Windows + Ubuntu)
- Shared app core loop
- Lua scripting integration via sol2
- Script hot-reload polling (`assets/scripts/main.lua`)
- SDL renderer path only

## Prerequisites
Install dependencies for your OS, then use the same CMake build flow.

### Windows (MSYS2 / MINGW64)
```bash
pacman -S --needed \
	mingw-w64-x86_64-toolchain \
	mingw-w64-x86_64-cmake \
	mingw-w64-x86_64-ninja \
	mingw-w64-x86_64-sdl3 \
	mingw-w64-x86_64-sdl3-ttf \
	mingw-w64-x86_64-lua \
	mingw-w64-x86_64-sol2
```

### Ubuntu
```bash
sudo apt update
sudo apt install -y \
	build-essential \
	cmake \
	ninja-build \
	libsdl3-dev \
	libsdl3-ttf-dev \
	lua5.4 \
	liblua5.4-dev \
	libsol2-dev
```

## Build (shared CMake flow)
```bash
cmake -S . -B build -G Ninja -DGUI_FETCH_DEPS=OFF
cmake --build build
./build/gui_pc
```

If local packages are unavailable and you want CMake to fetch missing dependencies from Git:

```bash
cmake -S . -B build -G Ninja -DGUI_FETCH_DEPS=ON
```

## Debug and run (VS Code)
- F5 debug profile: `Run GUI C++ (F5)`
- Debug binary path: `build-debug/gui_pc.exe`
- Release binary path: `build/gui_pc.exe`
- End-to-end runtime + debug workflow: see [docs/dev-loop.md](docs/dev-loop.md)

### Debug build (manual)
```bash
cmake -S . -B build-debug -G Ninja -DGUI_FETCH_DEPS=OFF -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
./build-debug/gui_pc
```

### Release build (manual)
```bash
cmake -S . -B build -G Ninja -DGUI_FETCH_DEPS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/gui_pc
```

### If F5 debugger fails
- Ensure `gdb` is installed and visible in PATH.
- Windows (MSYS2): verify `C:/msys64/mingw64/bin/gdb.exe` exists.
- Ubuntu: `sudo apt install -y gdb`.
- Re-run the debug task once: `Configure+Build GUI C++ (Debug)`.
- On Windows, confirm PATH includes `C:/msys64/mingw64/bin` (already set in `.vscode/launch.json`).
- If stale sessions interfere, kill all terminals/debug sessions and start F5 again.

## Typography font pin
- The renderer now prefers [assets/fonts/UiFont.ttf](assets/fonts/UiFont.ttf) for consistent text rendering.
- Fallback remains available (system fonts, then SDL debug text) if the bundled font is missing.
- If you redistribute the project, verify font licensing and replace `UiFont.ttf` with your preferred redistributable font if needed.

## Next step
Add renderer abstraction improvements and optional GPU path while preserving `IRenderer` API.
