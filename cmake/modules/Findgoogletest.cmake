include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY "https://github.com/google/googletest.git"
        GIT_TAG "release-1.12.1"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(googletest)