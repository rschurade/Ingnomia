add_executable(test_behaviortree
        main.cpp
        )

target_link_libraries(
        test_behaviortree
        Ingnomia::BehaviorTree
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(test_behaviortree)