set(TEST_SOURCES
        main.cpp
        test_decoder.cpp
)

add_executable(sim8086_tests ${TEST_SOURCES})
target_link_libraries(sim8086_tests PRIVATE sim8086_core gtest_main)
target_include_directories(sim8086_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set path to listings
add_test(NAME Sim8086Tests COMMAND sim8086_tests)
set_tests_properties(Sim8086Tests PROPERTIES
    ENVIRONMENT "TEST_DATA_DIR=${CMAKE_SOURCE_DIR}/listings"
)
