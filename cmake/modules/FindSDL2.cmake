include(FetchContent)

FetchContent_Declare(
        SDL2
        GIT_REPOSITORY "https://github.com/libsdl-org/SDL.git"
        GIT_TAG "release-2.0.22"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(SDL2)
