cmake_minimum_required(VERSION 3.10)
project(nvim-ui CXX)

set(VERSION 0.0.2-rc1)

find_package(Threads)
find_package(PkgConfig REQUIRED) 
pkg_check_modules(LIBUV REQUIRED IMPORTED_TARGET libuv)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)
pkg_check_modules(PANGO REQUIRED IMPORTED_TARGET pango)
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)

add_definitions(-DVERSION=\"${VERSION}\")
set(CMAKE_CXX_STANDARD 20)
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
    src/RedrawHandler.cpp
    src/Renderer.cpp
    src/TextureCache.cpp
    src/Timer.cpp
    src/Logger.cpp
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
    src/Painter.cpp
    src/Input.cpp
)
target_link_libraries(nvim-ui PUBLIC
    nvim-ui-lib
    PkgConfig::SDL2
    PkgConfig::CAIRO
    PkgConfig::PANGO
    PkgConfig::PANGOCAIRO
    )
target_sources(nvim-ui PRIVATE res/windows.rc)
install(TARGETS nvim-ui DESTINATION bin)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)

add_subdirectory(tests)
enable_testing()
add_test(tests tests/tests)
