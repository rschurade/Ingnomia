include(FetchContent)

FetchContent_Declare(
        STB
        GIT_REPOSITORY "https://github.com/nothings/stb.git"
        GIT_TAG "af1a5bc352164740c1cc1354942b1c6b72eacb8a")
FetchContent_MakeAvailable(STB)
