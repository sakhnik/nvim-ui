cmake_minimum_required(VERSION 3.10)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog"
    GIT_TAG "v1.8.2"
)
FetchContent_MakeAvailable(spdlog)

project(nvim-cur)
find_package(Threads)
add_definitions(-std=c++20 -Wall)
include_directories(${spdlog_SOURCE_DIR}/include)
add_executable(nvim-cur main.cpp)
target_link_libraries(nvim-cur ${CMAKE_THREAD_LIBS_INIT} -ltermkey -lboost_filesystem)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)
