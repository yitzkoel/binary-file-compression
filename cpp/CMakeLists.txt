cmake_minimum_required(VERSION 3.20)
project(cpp_compressor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_executable(cpp_main
        src/main.cpp
        src/binary_io.h
        src/binary_io.cpp
        src/tests/test_binary_io.cpp
        src/tests/test_binary_io.h
        src/lempel_ziv_algo.h
        src/lempel_ziv_algo.cpp
)