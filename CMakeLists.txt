cmake_minimum_required(VERSION 3.24)
project(gui_pc LANGUAGES CXX)

if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
  set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

option(GUI_FETCH_DEPS "Fetch dependencies from upstream if not found locally" ON)

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  if(NOT GUI_FETCH_DEPS)
    message(FATAL_ERROR "SDL3 not found and GUI_FETCH_DEPS=OFF")
  endif()
  FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.8
  )
  FetchContent_MakeAvailable(SDL3)
endif()

if(TARGET SDL3::SDL3)
  set(SDL3_TARGET SDL3::SDL3)
elseif(TARGET SDL3::SDL3-static)
  set(SDL3_TARGET SDL3::SDL3-static)
else()
  message(FATAL_ERROR "No compatible SDL3 target found")
endif()

set(GUI_HAS_SDL_TTF 0)
find_package(SDL3_ttf CONFIG QUIET)
if(SDL3_ttf_FOUND)
  if(TARGET SDL3_ttf::SDL3_ttf)
    set(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf)
    set(GUI_HAS_SDL_TTF 1)
  elseif(TARGET SDL3_ttf::SDL3_ttf-static)
    set(SDL3_TTF_TARGET SDL3_ttf::SDL3_ttf-static)
    set(GUI_HAS_SDL_TTF 1)
  endif()
endif()

if(NOT GUI_HAS_SDL_TTF)
  find_package(PkgConfig QUIET)
  if(PkgConfig_FOUND)
    pkg_check_modules(SDL3_TTF_PKG QUIET IMPORTED_TARGET SDL3_ttf)
    if(NOT SDL3_TTF_PKG_FOUND)
      pkg_check_modules(SDL3_TTF_PKG QUIET IMPORTED_TARGET sdl3-ttf)
    endif()

    if(SDL3_TTF_PKG_FOUND AND TARGET PkgConfig::SDL3_TTF_PKG)
      set(SDL3_TTF_TARGET PkgConfig::SDL3_TTF_PKG)
      set(GUI_HAS_SDL_TTF 1)
    endif()
  endif()
endif()

if(NOT GUI_HAS_SDL_TTF)
  find_path(SDL3_TTF_INCLUDE_DIR NAMES SDL3_ttf/SDL_ttf.h)
  find_library(SDL3_TTF_LIBRARY NAMES SDL3_ttf)

  if(SDL3_TTF_INCLUDE_DIR AND SDL3_TTF_LIBRARY)
    if(NOT TARGET gui_sdl3_ttf_imported)
      add_library(gui_sdl3_ttf_imported INTERFACE IMPORTED)
      target_include_directories(gui_sdl3_ttf_imported INTERFACE ${SDL3_TTF_INCLUDE_DIR})
      target_link_libraries(gui_sdl3_ttf_imported INTERFACE ${SDL3_TTF_LIBRARY})
    endif()

    set(SDL3_TTF_TARGET gui_sdl3_ttf_imported)
    set(GUI_HAS_SDL_TTF 1)
  endif()
endif()

find_package(sol2 CONFIG QUIET)
if(NOT sol2_FOUND)
  find_path(SOL2_INCLUDE_DIR NAMES sol/sol.hpp)
  if(SOL2_INCLUDE_DIR)
    if(NOT TARGET sol2::sol2_header_only)
      add_library(sol2::sol2_header_only INTERFACE IMPORTED)
      target_include_directories(sol2::sol2_header_only INTERFACE ${SOL2_INCLUDE_DIR})
    endif()
    set(sol2_FOUND TRUE)
  endif()
endif()

if(NOT sol2_FOUND)
  if(NOT GUI_FETCH_DEPS)
    message(FATAL_ERROR "sol2 not found and GUI_FETCH_DEPS=OFF")
  endif()
  FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
  )
  FetchContent_MakeAvailable(sol2)
endif()

if(TARGET sol2::sol2)
  set(SOL2_TARGET sol2::sol2)
elseif(TARGET sol2::sol2_header_only)
  set(SOL2_TARGET sol2::sol2_header_only)
else()
  message(FATAL_ERROR "No compatible sol2 target found")
endif()

find_package(Lua QUIET)
if(NOT Lua_FOUND)
  if(NOT GUI_FETCH_DEPS)
    message(FATAL_ERROR "Lua not found and GUI_FETCH_DEPS=OFF")
  endif()
  FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG v5.4.6
  )
  FetchContent_MakeAvailable(lua)
endif()

if(Lua_FOUND)
  if(TARGET Lua::Lua)
    set(LUA_TARGET Lua::Lua)
  else()
    if(NOT TARGET gui_lua_imported)
      add_library(gui_lua_imported INTERFACE IMPORTED)

      if(DEFINED LUA_INCLUDE_DIRS)
        target_include_directories(gui_lua_imported INTERFACE ${LUA_INCLUDE_DIRS})
      elseif(DEFINED LUA_INCLUDE_DIR)
        target_include_directories(gui_lua_imported INTERFACE ${LUA_INCLUDE_DIR})
      elseif(DEFINED Lua_INCLUDE_DIRS)
        target_include_directories(gui_lua_imported INTERFACE ${Lua_INCLUDE_DIRS})
      endif()

      if(DEFINED LUA_LIBRARIES)
        target_link_libraries(gui_lua_imported INTERFACE ${LUA_LIBRARIES})
      elseif(DEFINED Lua_LIBRARIES)
        target_link_libraries(gui_lua_imported INTERFACE ${Lua_LIBRARIES})
      else()
        message(FATAL_ERROR "Lua found but no library variables were provided by FindLua")
      endif()
    endif()

    set(LUA_TARGET gui_lua_imported)
  endif()
elseif(TARGET lua_static)
  set(LUA_TARGET lua_static)
elseif(TARGET lua)
  set(LUA_TARGET lua)
else()
  message(FATAL_ERROR "No Lua target was created by fetched Lua repository")
endif()

add_executable(gui_pc
  native/platform_sdl/main.cpp
  native/core/AppCore.cpp
  native/core/ScriptEngine.cpp
  native/renderer_sdl/RendererSdl.cpp
)

target_include_directories(gui_pc PRIVATE
  ${CMAKE_SOURCE_DIR}/native
)

target_link_libraries(gui_pc PRIVATE
  ${SDL3_TARGET}
  ${SOL2_TARGET}
  ${LUA_TARGET}
)

if(GUI_HAS_SDL_TTF)
  target_link_libraries(gui_pc PRIVATE ${SDL3_TTF_TARGET})
endif()

target_compile_definitions(gui_pc PRIVATE GUI_HAS_SDL_TTF=${GUI_HAS_SDL_TTF})
