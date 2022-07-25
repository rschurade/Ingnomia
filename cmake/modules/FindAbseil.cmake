include(FetchContent)

set(BUILD_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
set(ABSL_ENABLE_INSTALL OFF)
set(ABSL_PROPAGATE_CXX_STD ON)
FetchContent_Declare(
    absl
    GIT_REPOSITORY "https://github.com/abseil/abseil-cpp.git"
    GIT_TAG "20220623.rc1"
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(absl)
