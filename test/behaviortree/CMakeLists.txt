add_executable(test_behaviortree
        main.cpp
        )

target_link_libraries(
        test_behaviortree
        Ingnomia::BehaviorTree
        GTest::gtest_main
)

# Deployment helpers for Windows
if(WIN32)
    include(windeployqt)
    windeployqt(test_behaviortree)
endif()

include(GoogleTest)
gtest_discover_tests(test_behaviortree)