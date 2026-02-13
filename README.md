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

## Android / NDK scaffold
- A minimal Android NativeActivity scaffold is available under [android](android).
- Open [android](android) in Android Studio and let Gradle sync.
- In Android Studio, use the Gradle Wrapper from the project (do not force a local Gradle 9+ install).
- Build/run the `app` module (arm64-v8a).
- Native bootstrap target is defined in [android/app/src/main/cpp/CMakeLists.txt](android/app/src/main/cpp/CMakeLists.txt).
- Shared core wiring is now connected incrementally (`AppCore` linked into Android target) using a temporary Android script-engine stub in [android/app/src/main/cpp/ScriptEngineStub.cpp](android/app/src/main/cpp/ScriptEngineStub.cpp).
- Current scaffold is intentionally minimal and ready for the next pass: replacing the stub with real Lua/sol2 runtime on Android.

### Android text input / OSK note (SDL + NDK)
- Yes, for text widgets on Android you should use the on-screen keyboard (OSK).
- In SDL, show keyboard on text-focus by calling `SDL_StartTextInput(window)` and hide it on blur with `SDL_StopTextInput(window)`.
- Continue handling text through `SDL_EVENT_TEXT_INPUT`; keep key events for control keys (Backspace/Enter/etc).
- If no text field is focused, keep OSK hidden.
- If you add custom caret/selection UX, also update text-input area as needed (`SDL_SetTextInputArea`) so IME behavior stays correct.

### Immediate next Android pass
- Replace [android/app/src/main/cpp/ScriptEngineStub.cpp](android/app/src/main/cpp/ScriptEngineStub.cpp) with real Lua/sol2-backed `ScriptEngine` on Android.
- Add Android asset loading for `assets/scripts/main.lua` and related resources.
- Introduce SDL Android host path and wire focus-driven OSK behavior for text widgets.

## Next step
Add renderer abstraction improvements and optional GPU path while preserving `IRenderer` API.
