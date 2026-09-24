cmake_minimum_required(VERSION 3.20)
project(cpp_compressor CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")


# The core compressing library
add_library(compressor_lib
        src/LempelZivCodec.cpp
        src/BinaryIO.cpp
        src/huffmanCodec.cpp
        src/hashTable.cpp
        src/HuffmanBuilder.cpp
        src/BitWriter.cpp
        src/BitReader.cpp
)

# Linking compressor_lib to the include folder
target_include_directories(compressor_lib PUBLIC include)


# The main target
add_executable(cpp_main
        src/main.cpp
)

# Linking the compressor library to main
target_link_libraries(cpp_main PRIVATE compressor_lib)

enable_testing()

# add the CMakeLists.txt in tests directory
add_subdirectory(tests)