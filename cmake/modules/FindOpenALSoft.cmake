include(FetchContent)

set(ALSOFT_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        OpenALSoft
        GIT_REPOSITORY "https://github.com/kcat/openal-soft.git"
        GIT_TAG "1.22.2"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(OpenALSoft)
