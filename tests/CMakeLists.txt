
include(FetchContent)
FetchContent_Declare(
    ut
    GIT_REPOSITORY "https://github.com/boost-ext/ut"
    GIT_TAG "v1.1.8"
)
# FetchContent_MakeAvailable(ut)   # can't do that because ut is being installed with "make install"
FetchContent_GetProperties(ut)
if(NOT ut_POPULATED)
  FetchContent_Populate(ut)
  add_subdirectory(${ut_SOURCE_DIR} ${ut_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

include_directories(${ut_SOURCE_DIR}/include)
add_executable(tests
    test.cpp
    Renderer.cpp
    TextureCache.cpp
)
target_link_libraries(tests nvim-sdl2-lib)
