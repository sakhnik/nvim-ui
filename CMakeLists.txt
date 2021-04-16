cmake_minimum_required(VERSION 3.10)
project(nvim-sdl2 CXX)

set(VERSION 0.0.1)

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

add_library(nvim-sdl2-lib STATIC
    src/MsgPackRpc.cpp
    src/RedrawHandler.cpp
    src/Renderer.cpp
    src/TextureCache.cpp
    src/Timer.cpp
)
target_include_directories(nvim-sdl2-lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nvim-sdl2-lib PUBLIC
    ${CMAKE_THREAD_LIBS_INIT}
    PkgConfig::LIBUV
    )

add_executable(nvim-sdl2
    src/main.cpp
    src/Window.cpp
    src/Painter.cpp
    src/Input.cpp
)
target_link_libraries(nvim-sdl2 PUBLIC
    nvim-sdl2-lib
    PkgConfig::SDL2
    PkgConfig::CAIRO
    PkgConfig::PANGO
    PkgConfig::PANGOCAIRO
    )
target_sources(nvim-sdl2 PRIVATE res/windows.rc)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)

add_subdirectory(tests)
enable_testing()
add_test(tests tests/tests)
