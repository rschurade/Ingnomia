include(FetchContent)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(SDL2IMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        SDL2_Image
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
        GIT_TAG "release-2.6.0"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SDL2_Image)
