cmake_minimum_required(VERSION 3.22.1)
project(guicpp_native LANGUAGES CXX)

include(FetchContent)

add_library(guicpp_native SHARED
    bootstrap.cpp
    ../../../../../native/core/AppCore.cpp
    ../../../../../native/core/ScriptEngine.cpp
)

target_compile_features(guicpp_native PRIVATE cxx_std_20)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG v3.5.0
)
FetchContent_GetProperties(sol2)
if(NOT sol2_POPULATED)
    FetchContent_Populate(sol2)
endif()

if(NOT TARGET guicpp_sol2_headers)
    add_library(guicpp_sol2_headers INTERFACE)
    target_include_directories(guicpp_sol2_headers INTERFACE ${sol2_SOURCE_DIR}/include)
endif()

FetchContent_Declare(
    lua
    GIT_REPOSITORY https://github.com/walterschell/Lua.git
    GIT_TAG v5.4.6
)
FetchContent_MakeAvailable(lua)

if(TARGET lua_static)
    set(LUA_TARGET lua_static)
elseif(TARGET lua)
    set(LUA_TARGET lua)
else()
    message(FATAL_ERROR "No Lua target was created by fetched Lua repository")
endif()

target_include_directories(guicpp_native PRIVATE
    ../../../../../native
)

find_library(log-lib log)

target_link_libraries(guicpp_native PRIVATE
    android
    ${log-lib}
    guicpp_sol2_headers
    ${LUA_TARGET}
)
