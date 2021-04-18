
include(FetchContent)
FetchContent_Declare(
    ut
    GIT_REPOSITORY "https://github.com/boost-ext/ut"
    GIT_TAG "v1.1.8"
)
FetchContent_MakeAvailable(ut)

include_directories(${ut_SOURCE_DIR}/include)
add_executable(tests
    test.cpp
    Renderer.cpp
    TextureCache.cpp
)
target_link_libraries(tests nvim-sdl2-lib)
