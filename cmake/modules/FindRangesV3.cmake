include(FetchContent)

FetchContent_Declare(
    rangesv3
    GIT_REPOSITORY "https://github.com/ericniebler/range-v3.git"
    GIT_TAG "0.12.0")
FetchContent_MakeAvailable(rangesv3)
