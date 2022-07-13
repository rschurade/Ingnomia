include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
        GIT_TAG "v1.10.0")
FetchContent_MakeAvailable(spdlog)