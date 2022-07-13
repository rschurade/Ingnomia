include(FetchContent)

FetchContent_Declare(
        pugixml
        GIT_REPOSITORY "https://github.com/zeux/pugixml.git"
        GIT_TAG "2639dfd053221d3e8c9e9ff013e58699d9c1af15")
FetchContent_MakeAvailable(pugixml)