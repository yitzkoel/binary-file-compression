include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)


FetchContent_MakeAvailable(googletest)

add_executable(run_unit_tests
        test_LempelZivCodec.cpp
        test_HashTable.cpp
        test_huffmanBuilder.cpp
        test_BitWriter.cpp
        test_BitReader.cpp
        test_HuffmanCodec.cpp
)

target_link_libraries(run_unit_tests
        PRIVATE
        compressor_lib
        GTest::gtest_main
)


include(GoogleTest)
gtest_discover_tests(run_unit_tests)