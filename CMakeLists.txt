cmake_minimum_required(VERSION 3.10)
project(nvim-sdl2)

find_package(Threads)
find_package(PkgConfig REQUIRED) 
pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)

add_definitions(-std=c++17 -Wall)
if(${MINGW})
    add_definitions(-mwindows)
endif()

add_executable(nvim-sdl2
    src/main.cpp
    src/MsgPackRpc.cpp
    src/RedrawHandler.cpp
    src/Renderer.cpp
    src/Painter.cpp
    src/Input.cpp
)
target_link_libraries(nvim-sdl2 PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    PkgConfig::LIBUV
    PkgConfig::SDL2
    PkgConfig::CAIRO
    PkgConfig::PANGO
    PkgConfig::PANGOCAIRO
    )
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)
