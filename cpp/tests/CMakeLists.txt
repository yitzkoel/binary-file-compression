include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


FetchContent_MakeAvailable(googletest)

add_executable(run_unit_tests
                test_lempel_ziv.cpp
)

target_link_libraries(run_unit_tests
        PRIVATE
        compressor_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_unit_tests)