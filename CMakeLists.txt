cmake_minimum_required(VERSION 3.10)
project(nvim-ui C CXX)

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

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Window.cpp
)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources REQUIRED)

set(GRESOURCE_C   res.c)
set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/res/res.xml)

add_custom_command(
    OUTPUT ${GRESOURCE_C}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res
    COMMAND ${GLIB_COMPILE_RESOURCES}
    ARGS
        --generate
        --target=${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C}
        ${GRESOURCE_XML}
    VERBATIM
    MAIN_DEPENDENCY ${GRESOURCE_XML}
    DEPENDS
        res/nvim-ui.svg
)

target_sources(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/res/windows.rc
    ${CMAKE_CURRENT_BINARY_DIR}/${GRESOURCE_C}
)

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(${PROJECT_NAME} PUBLIC
    nvim-ui-lib
    PkgConfig::GLIB
    PkgConfig::GTK
    )
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
#target_precompile_headers(nvim-cur PRIVATE pch.hpp)

add_subdirectory(tests)
enable_testing()
add_test(tests tests/tests)
