include(FetchContent)

FetchContent_Declare(sqlite3
        URL "https://sqlite.org/2022/sqlite-amalgamation-3390000.zip"
        URL_HASH MD5=e2a318403d402d751fa2359ac6368751
        )
FetchContent_MakeAvailable(sqlite3)
