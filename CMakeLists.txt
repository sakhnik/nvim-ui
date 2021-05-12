cmake_minimum_required(VERSION 3.10)
project(nvim-ui CXX)

set(VERSION 0.0.3)

find_package(Threads)
find_package(PkgConfig REQUIRED) 
pkg_check_modules(MSGPACK REQUIRED IMPORTED_TARGET msgpack)
pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GTK REQUIRED IMPORTED_TARGET gtk4)

add_definitions(-DVERSION=\"${VERSION}\")
set(CMAKE_CXX_STANDARD 17)
add_compile_options(-Wall)

##################################################
# spdlog

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY "https://github.com/gabime/spdlog"
    GIT_TAG "v1.8.5"
)
# FetchContent_MakeAvailable(spdlog)   # can't do that because ut is being installed with "make install"
FetchContent_GetProperties(spdlog)
if(NOT spdlog_POPULATED)
  FetchContent_Populate(spdlog)
  add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
include_directories(${spdlog_SOURCE_DIR}/include)

##################################################
#
add_library(nvim-ui-lib STATIC
    src/MsgPackRpc.cpp
    src/AsyncExec.cpp
    src/RedrawHandler.cpp
    src/Renderer.cpp
    src/TextureCache.cpp
    src/Timer.cpp
    src/Logger.cpp
    src/Input.cpp
)
target_include_directories(nvim-ui-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nvim-ui-lib PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    PkgConfig::LIBUV
    spdlog::spdlog
    )

add_executable(nvim-ui
    src/main.cpp
    src/Window.cpp
)
target_link_libraries(nvim-ui PUBLIC
    nvim-ui-lib
    PkgConfig::GLIB
    PkgConfig::GTK
    )
target_sources(nvim-ui PRIVATE res/windows.rc)
install(TARGETS nvim-ui DESTINATION bin)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)

add_subdirectory(tests)
enable_testing()
add_test(tests tests/tests)
