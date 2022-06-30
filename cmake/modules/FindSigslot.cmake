include(FetchContent)

FetchContent_Declare(
    sigslot
    GIT_REPOSITORY "https://github.com/palacaze/sigslot"
    GIT_TAG "f8c275df5037bfb6d57c74168634b65fdbfb44f3")
FetchContent_MakeAvailable(sigslot)
