include(FetchContent)

set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_OPENGL_VERSION 41 CACHE STRING "Set BGFX OpenGL version to 4.1 (max version on macOS)" FORCE)
FetchContent_Declare(
        BGFX
        GIT_REPOSITORY "https://github.com/bkaradzic/bgfx.cmake.git"
        GIT_TAG "896ab5478cd943c51fe14ff8e4a1abf7b4102bca")
FetchContent_MakeAvailable(BGFX)