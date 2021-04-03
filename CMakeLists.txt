cmake_minimum_required(VERSION 3.10)

project(nvim-cur)
find_package(Threads)
add_definitions(-std=c++20 -Wall)
add_executable(nvim-cur main.cpp)
target_link_libraries(nvim-cur ${CMAKE_THREAD_LIBS_INIT} -ltermkey -luv -lSDL2 -lSDL2_ttf)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)
