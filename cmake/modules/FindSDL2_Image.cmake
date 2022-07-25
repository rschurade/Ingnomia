include(FetchContent)

FetchContent_Declare(
        SDL2_Image
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL_image.git"
        GIT_TAG "release-2.6.0"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SDL2_Image)
