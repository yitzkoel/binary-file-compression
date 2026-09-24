//
// Created by yitzk on 8/14/2026.
//

#include "../include/LempelZivCodec.h"
#include "../include/BinaryIO.h"
#include <string>
#include <gtest/gtest.h>
#include <fstream>
#include <utility>
#include <filesystem>
#include <memory>

/**
 * @class LempelZivCodecTest
 * @brief Test fixture for the Lempel-Ziv (LZSS) compression algorithm.
 *
 * Provides helper methods to manipulate the internal state of the compressor
 * for unit testing private methods, and verifies the end-to-end compression
 * and decompression cycles using the memory-buffer API.
 */
class LempelZivCodecTest : public ::testing::Test
{
protected:
    LempelZivCodec compressor;
    uint32_t WINDOW_OFFSET;
    inline static const uint16_t EOF_SYMBOL = 256;

    LempelZivCodecTest():
        compressor(), WINDOW_OFFSET(LempelZivCodec::WINDOW_OFFSET)
    {
    }

    // ==========================================
    // Internal State Mutators for Unit Testing
    // ==========================================

    void set_literal(uint64_t literal)
    {
        compressor.literal = literal;
    }

    void set_window(uint64_t start_window_index, uint64_t len_window)
    {
        compressor.start_window_index = start_window_index;
        compressor.len_window = len_window;
    }

    void add_window_to_vec(CodedVec& coded_vec) const
    {
        coded_vec.push_back((compressor.len_window));
        coded_vec.push_back((compressor.start_window_index));
    }

    void add_literal_to_vec(CodedVec& coded_vec) const
    {
        coded_vec.push_back(compressor.literal);
    }

    void set_buffer(std::vector<uint8_t>& data, uint8_t* buffer)
    {
        compressor.buffer = buffer;
        for (size_t i = 0; i < data.size(); i++)
        {
            compressor.buffer[i] = data[i];
        }
        compressor.num_bytes_in_buffer = data.size();
    }

    void set_index_in_buffer(uint64_t index)
    {
        compressor.index_in_buffer = index;
    }

    // ==========================================
    // Internal State Accessors for Unit Testing
    // ==========================================

    uint64_t get_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size)
    {
        return compressor.find_max_window_from_given_index(index_to_start_searching, max_window_size);
    }

    bool find_window()
    {
        return compressor.find_window();
    }

    [[nodiscard]] uint64_t get_window_size() const
    {
        return compressor.len_window;
    }

    [[nodiscard]] uint64_t get_start_index() const
    {
        return compressor.start_window_index;
    }

    [[nodiscard]] uint64_t get_literal() const
    {
        return compressor.literal;
    }

    [[nodiscard]] uint64_t get_current_index_in_buffer() const
    {
        return compressor.index_in_buffer;
    }

    void increment_index_in_buffer(uint64_t val_to_increment_with)
    {
        compressor.index_in_buffer += val_to_increment_with;
    }

    const cyclicArray& get_array(uint32_t key)
    {
        return *(compressor.hash_map.find(key));
    }

    [[nodiscard]] bool is_key_in_dict(uint32_t key)
    {
        return compressor.hash_map.find(key) != nullptr;
    }
};

TEST_F(LempelZivCodecTest, AddingToCodedVec)
{
    std::cout << "[INFO] Testing basic vector insertions for LZSS symbols.\n";

    CodedVec coded_vec;

    this->set_window(50, 25);
    this->add_window_to_vec(coded_vec);

    this->set_literal(100);
    this->add_literal_to_vec(coded_vec);

    // Verify LIFO retrieval matches expected insertion order
    EXPECT_EQ(coded_vec.back(), 100);
    coded_vec.pop_back();
    EXPECT_EQ(coded_vec.back(), 50);
    coded_vec.pop_back();
    EXPECT_EQ(coded_vec.back(), 25);
    coded_vec.pop_back();

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestFindMaxWindowFromGivenIndex)
{
    std::cout << "[INFO] Testing window matching logic across various boundaries and overlaps.\n";

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();

    // Basic tests for exact matches and shorter limits
    std::vector<uint8_t> buffer_data = {1, 0, 0, 0, 1, 0, 0, 0};
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(4);

    EXPECT_EQ(get_max_window_from_given_index(0, 4), 4);
    EXPECT_EQ(get_max_window_from_given_index(1, 4), 0);
    EXPECT_EQ(get_max_window_from_given_index(0, 3), 3);
    EXPECT_EQ(get_max_window_from_given_index(0, 2), 2);
    EXPECT_EQ(get_max_window_from_given_index(0, 1), 1);

    // Test searching for a past window that is exactly 32 bytes
    buffer_data.clear();
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 32; i++) { buffer_data.push_back(i); }
    }
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(32);
    EXPECT_EQ(get_max_window_from_given_index(0, 32), 32);

    // Test searching for a past window that is more than 32 bytes
    buffer_data.clear();
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 33; i++) { buffer_data.push_back(i); }
    }
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(33);
    EXPECT_EQ(get_max_window_from_given_index(0, 33), 33);

    // Test searching for a past window that overflows into the future (overlapping match)
    buffer_data = {1, 0, 1, 0, 1, 0};
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(2);
    EXPECT_EQ(get_max_window_from_given_index(0, 4), 4);

    buffer_data.clear();
    for (int i = 0; i < 50; i++) { buffer_data.push_back(0); }
    set_index_in_buffer(10);
    set_buffer(buffer_data, buff_ptr->data());
    EXPECT_EQ(get_max_window_from_given_index(9, 40), 40);

    // Test for a match that is smaller than the requested max window size
    buffer_data.clear();
    int k = 0;
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 50; i++)
        {
            if (i < 15) buffer_data.push_back(i);
            else buffer_data.push_back(k);
        }
        k++;
    }
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(50);
    EXPECT_EQ(get_max_window_from_given_index(0, 50), 15);

    // Test requesting a max window of size 0
    buffer_data = {1, 0, 1, 0, 1, 0};
    set_buffer(buffer_data, buff_ptr->data());
    set_index_in_buffer(2);
    EXPECT_EQ(get_max_window_from_given_index(0, 0), 0);

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestFindWindowBasicFlow)
{
    std::cout << "[INFO] Testing core compression loop and sequential match finding.\n";

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();
    std::vector<uint8_t> buffer_data = {'a', 'a', 'b', 'c', 'a', 'a', 'b', 'c', 'd', 'a', 'a', 'b', 'c', 'a', 'e'};
    uint32_t next_four_bytes = 0;

    set_buffer(buffer_data, buff_ptr->data());

    // Iterations 1-4: Accumulating history, no matches long enough yet
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 0);
    increment_index_in_buffer(1);

    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 1);
    increment_index_in_buffer(1);

    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'b');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 2);
    increment_index_in_buffer(1);

    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'c');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 3);
    increment_index_in_buffer(1);

    // Iteration 5: First full match 'a a b c' found
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 4);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 0);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[1], 4);
    increment_index_in_buffer(4);

    // Iteration 6: Literal 'd' breaks the pattern
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'd');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 8);
    increment_index_in_buffer(1);

    // Iteration 7: Second match 'a a b c a'
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 5);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 0);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[1], 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[2], 9);
    increment_index_in_buffer(5);

    // Iteration 8: Literal 'e' ends the stream
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'e');

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestFindWindow_OverlapFuture)
{
    std::cout << "[INFO] Testing overlapping future matches (RLE style repeating sequences).\n";

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();
    std::vector<uint8_t> buffer_data = {'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b'};
    uint32_t next_four_bytes = 0;

    set_buffer(buffer_data, buff_ptr->data());

    // Feed initial non-repeating data
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    increment_index_in_buffer(1);

    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'b');
    increment_index_in_buffer(1);

    // The rest of the buffer acts as a continuously overlapping match
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 12);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[0], 0);
    EXPECT_EQ(get_array(next_four_bytes).array_ptr[1], 2);
    increment_index_in_buffer(12);

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestCyclic_Array_Overflow)
{
    std::cout << "[INFO] Testing hash map cyclic array overflow replacement logic.\n";

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();
    std::vector<uint8_t> buffer_data;

    for (int i = 0; i < 33; i++)
    {
        for (int j = 0; j < 4; j++) { buffer_data.push_back(0); }
        buffer_data.push_back(i + 1);
    }
    set_buffer(buffer_data, buff_ptr->data());

    for (int i = 0; i < 69; i++)
    {
        find_window();
        if (i <= 4)
        {
            increment_index_in_buffer(1);
        }
        else if (get_current_index_in_buffer() % 5 == 0)
        {
            increment_index_in_buffer(4);
        }
        else
        {
            increment_index_in_buffer(1);
        }
    }

    auto cyclic_arr = get_array(0);
    for (int i = 1; i < 33; i++)
    {
        EXPECT_EQ(cyclic_arr.array_ptr[i % 32], 5 * i);
    }

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestBasicCompression)
{
    std::cout << "[INFO] Basic Compression Test: Validates LZSS encoding directly from a memory buffer.\n";

    std::vector<uint8_t> buffer_data = {
        'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'c', 'd', 'c', 'd', 'c', 'd', 'd',
        'a', 'b', 'c', 'e', 'a', 'b', 'c', 'e', 'd', 'a', 'a', 'b', 'a', 'a', 'c', 'f', 'g'
    };

    std::vector<uint32_t> expected_coded_vec = {
        'a', 'a', 'b',  5 + WINDOW_OFFSET, 3, 'c', 'd',  4 + WINDOW_OFFSET, 2, 'd', 'a', 'b', 'c', 'e',
         4 + WINDOW_OFFSET, 4, 'd', 6 + WINDOW_OFFSET, 21, 'f', 'g', EOF_SYMBOL
    };

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();

    // Copy the raw uncompressed data into our target buffer to simulate incoming file data
    std::memcpy(buff_ptr->data(), buffer_data.data(), buffer_data.size());

    // Compress directly from memory using the new API
    auto res_coded_vec = compressor.compress(buff_ptr->data(), buffer_data.size());

    EXPECT_EQ(expected_coded_vec.size(), res_coded_vec.size());
    for (size_t i = 0; i < expected_coded_vec.size(); i++)
    {
        EXPECT_EQ(expected_coded_vec[i], res_coded_vec[i]);
    }

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestBasicDecompression)
{
    std::cout << "[INFO] Basic Decompression Test: Validates decoding an LZSS vector directly into a memory buffer.\n";

    std::vector<uint32_t> coded_vec = {
        'a', 'a', 'b',  5 + WINDOW_OFFSET, 3, 'c', 'd',  4 + WINDOW_OFFSET, 2, 'd', 'a', 'b', 'c', 'e',
         4 + WINDOW_OFFSET, 4, 'd', 6 + WINDOW_OFFSET, 21, 'f', 'g', EOF_SYMBOL
    };

    std::vector<uint8_t> expected_data = {
        'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'c', 'd', 'c', 'd', 'c', 'd', 'd',
        'a', 'b', 'c', 'e', 'a', 'b', 'c', 'e', 'd', 'a', 'a', 'b', 'a', 'a', 'c', 'f', 'g'
    };

    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buff_ptr = std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();

    // Decompress the LZSS vector directly into our allocated buffer using the new API
    uint64_t bytes_written = compressor.decompress(buff_ptr->data(), BUFFER_SIZE, coded_vec);

    EXPECT_EQ(bytes_written, expected_data.size());

    for (size_t i = 0; i < expected_data.size(); i++)
    {
        EXPECT_EQ(buff_ptr->data()[i], expected_data[i]);
    }

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(LempelZivCodecTest, TestOnRealFilesCompressionAndDecompression)
{
    std::cout << "[INFO] Real Files End-to-End Test: Compress and decompress physical files ensuring lossless memory operations.\n";

    std::vector<std::string> samp_files_path_name_vec = {
        "../../tests/test_files/Samp1.bin",
        "../../tests/test_files/Samp2.bin",
        "../../tests/test_files/Samp3.bin",
        "../../tests/test_files/Samp4.bin",
        "../../tests/test_files/Samp5.bin"
    };

    for (const auto& file_name : samp_files_path_name_vec)
    {
        std::string base_name = std::filesystem::path(file_name).filename().string();
        std::string res_file_name = "res_" + base_name;

        // Reset compressor state for a clean cycle
        compressor.clear();

        // Load file chunk into memory
        BinaryIO::FileReader file_reader(file_name);
        file_reader.slide_window();

        // Use get_num_bytes_read() to accurately pass the exact data size, preventing trailing garbage compression
        auto code_vec = compressor.compress(file_reader.get_buffer(), file_reader.get_num_bytes_read());

        // Decompress the generated vector into a new output buffer
        BinaryIO::FileWriter file_writer(res_file_name);
        auto num_bytes_written = compressor.decompress(file_writer.get_buffer(), BUFFER_SIZE, code_vec);

        // Write the decoded buffer to disk for verification
        file_writer.flush_buffer_to_file(num_bytes_written);

        EXPECT_EQ(std::filesystem::file_size(file_name), std::filesystem::file_size(res_file_name));

        // Deep byte-by-byte file integrity verification
        BinaryIO::FileReader file(file_name);
        BinaryIO::FileReader res_file(res_file_name);

        while (file.slide_window() && res_file.slide_window())
        {
            auto file_buffer = file.get_buffer();
            auto res_file_buffer = res_file.get_buffer();

            for(size_t j = 0; j < file.get_num_bytes_read(); j++)
            {
                EXPECT_EQ(file_buffer[j], res_file_buffer[j]);
            }
        }
    }

    std::cout << "*************** Finish test ***********\n\n";
}