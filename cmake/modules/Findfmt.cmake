include(FetchContent)

FetchContent_Declare(
        fmt
        GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
        GIT_TAG "8.1.1"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(fmt)
