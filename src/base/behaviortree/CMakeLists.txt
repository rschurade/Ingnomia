file(GLOB_RECURSE SOURCE_LIST
        "*.h"
        "*.hpp"
        "*.cpp"
)

add_library(IngnomiaBT
        STATIC EXCLUDE_FROM_ALL
        ${SOURCE_LIST}
        ${CMAKE_CURRENT_LIST_FILE}
)

target_link_libraries(IngnomiaBT
        Qt5::Core
        Qt5::Xml

        absl::hash
        absl::flat_hash_map
)

add_library(Ingnomia::BehaviorTree ALIAS IngnomiaBT)

