cmake_minimum_required(VERSION 3.10)

project(nvim-sdl2)
find_package(Threads)
add_definitions(-std=c++17 -Wall)
add_executable(nvim-sdl2
    src/main.cpp
    src/MsgPackRpc.cpp
    src/RedrawHandler.cpp
    src/Renderer.cpp
    src/Input.cpp
)
target_link_libraries(nvim-sdl2 ${CMAKE_THREAD_LIBS_INIT} -luv -lSDL2 -lSDL2_ttf)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)
