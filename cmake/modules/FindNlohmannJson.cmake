include(FetchContent)

FetchContent_Declare(json
        URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz
        URL_HASH MD5=4b67aba51ddf17c798e80361f527f50e
)
FetchContent_MakeAvailable(json)