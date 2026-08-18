//
// Created by yitzk on 8/14/2026.
//
#include "../include/lempel_ziv_algo.h"
#include <string>
#include<gtest/gtest.h>
#include <fstream>
#include <utility>
#include <filesystem>


class LempelZivTest : public ::testing::Test
{
protected:
    Lempel_ziv_algo compressor;

    LempelZivTest():
        compressor()
    {
        compressor.buffer = std::make_shared<std::array<uint8_t, BUFFER_SIZE>>();
    }

    void set_literal(uint64_t literal)
    {
        compressor.literal = literal;
    }

    void set_window(uint64_t start_window_index, uint64_t len_window)
    {
        compressor.start_window_index = start_window_index;
        compressor.len_window = len_window;
    }

    void add_window_to_vec()
    {
        compressor.add_window_to_vec();
    }

    void add_literal_to_vec()
    {
        compressor.add_literal_to_vec();
    }

    std::vector<uint64_t>& get_coded_vec()
    {
        return compressor.coded_vec;
    }

    std::vector<uint64_t>& get_bit_map()
    {
        return compressor.bit_map;
    }

    void set_coded_vec(std::vector<uint64_t>& coded_vec)
    {
        compressor.coded_vec = std::move(coded_vec);
    }

    void set_bit_map(std::vector<uint64_t>& bit_map)
    {
        compressor.bit_map = std::move(bit_map);
    }


    static uint64_t get_min(uint64_t val1, uint64_t val2)
    {
        return Lempel_ziv_algo::min(val1, val2);
    }

    static void create_input_file(const std::string& content, const std::string& file_path)
    {
        auto file = std::ofstream(file_path);
        file.write(content.c_str(), content.size());
        file.close();
    }

    void set_buffer(std::vector<uint8_t>& data)
    {
        for (int i = 0; i < data.size(); i++)
        {
            (*compressor.buffer)[i] = data[i];
        }

        compressor.num_bytes_read = data.size();
    }

    void set_index_in_buffer(uint64_t index)
    {
        compressor.index_in_buffer = index;
    }

    uint64_t get_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size)
    {
        return compressor.find_max_window_from_given_index(index_to_start_searching, max_window_size);
    }

    bool find_window()
    {
        return compressor.find_window();
    }

    uint64_t get_window_size() const
    {
        return compressor.len_window;
    }

    uint64_t get_start_index() const
    {
        return compressor.start_window_index;
    }

    uint64_t get_literal() const
    {
        return compressor.literal;
    }

    uint64_t get_current_index_in_buffer() const
    {
        return compressor.index_in_buffer;
    }

    void increment_index_in_buffer(uint64_t val_to_increment_with)
    {
        compressor.index_in_buffer += val_to_increment_with;
    }

    const cyclicArray& get_array(uint32_t key)
    {
        return compressor.hash_map[key];
    }

    bool is_key_in_dict(uint32_t key) const
    {
        return compressor.hash_map.find(key) != compressor.hash_map.end();
    }


};

TEST_F(LempelZivTest, AddingToCodedVec)
{
    this->set_window(50, 25);
    this->add_window_to_vec();


    this->set_literal(100);
    this->add_literal_to_vec();

    auto coded_vec = this->get_coded_vec();

    EXPECT_EQ(coded_vec.back(), 100);
    coded_vec.pop_back();
    EXPECT_EQ(coded_vec.back(), 25);
    coded_vec.pop_back();
    EXPECT_EQ(coded_vec.back(), 50);
    coded_vec.pop_back();
}

TEST_F(LempelZivTest, TestMin)
{
    EXPECT_EQ(get_min(1,0), 0);
    EXPECT_EQ(get_min(0,1), 0);
    EXPECT_EQ(get_min(0,0), 0);
    EXPECT_EQ(get_min(100000, 1ULL << 63), 100000);

    // testing the largest 64 number with the smallest
    EXPECT_EQ(get_min(0, -1), 0);
}

TEST_F(LempelZivTest, TestFindMaxWindowFromGivenIndex)
{
    // basic tests
    std::vector<uint8_t> buffer_data = {1, 0, 0, 0, 1, 0, 0, 0};
    set_buffer(buffer_data);

    set_index_in_buffer(4);

    EXPECT_EQ(get_max_window_from_given_index(0,4), 4);
    EXPECT_EQ(get_max_window_from_given_index(1,4), 0);
    EXPECT_EQ(get_max_window_from_given_index(0,3), 3);
    EXPECT_EQ(get_max_window_from_given_index(0,2), 2);
    EXPECT_EQ(get_max_window_from_given_index(0,1), 1);

    // test searching for past window more that is exactly 32 bytes
    buffer_data.clear();

    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 32; i++)
        {
            buffer_data.push_back(i);
        }
    }

    set_buffer(buffer_data);
    set_index_in_buffer(32);
    EXPECT_EQ(get_max_window_from_given_index(0,32), 32);

    // test searching for past window more that more than 32 bytes

    buffer_data.clear();
    for (int j = 0; j < 2; j++)
    {
        for (int i = 0; i < 33; i++)
        {
            buffer_data.push_back(i);
        }
    }
    set_buffer(buffer_data);
    set_index_in_buffer(33);
    EXPECT_EQ(get_max_window_from_given_index(0,33), 33);

    // test searching for past window more that overflows into the future
    buffer_data = {1, 0, 1, 0, 1, 0};
    set_buffer(buffer_data);
    set_index_in_buffer(2);
    EXPECT_EQ(get_max_window_from_given_index(0,4), 4);

    buffer_data.clear();
    for (int i = 0; i < 50; i++)
    {
        buffer_data.push_back(0);
    }
    set_index_in_buffer(10);
    set_buffer(buffer_data);
    EXPECT_EQ(get_max_window_from_given_index(9,40), 40);


    //test for match that is smaller than window
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
    set_buffer(buffer_data);
    set_index_in_buffer(50);
    EXPECT_EQ(get_max_window_from_given_index(0,50), 15);

    // test max window of size 0
    buffer_data = {1, 0, 1, 0, 1, 0};
    set_buffer(buffer_data);
    set_index_in_buffer(2);
    EXPECT_EQ(get_max_window_from_given_index(0,0), 0);
}

TEST_F(LempelZivTest, TestcyclicArray)
{
    cyclicArray arr;

    for (int i = 0; i < arr.array.size(); i++)
    {
        arr.add_elem(i);
    }

    EXPECT_EQ(arr.index_in_array, 0);

    arr.add_elem(5);

    EXPECT_EQ(arr.array[0], 5);
}


TEST_F(LempelZivTest, TestFindWindowBasicFlow)
{
    std::vector<uint8_t> buffer_data =
        {'a', 'a', 'b', 'c', 'a', 'a', 'b', 'c', 'd', 'a', 'a', 'b', 'c', 'a', 'e'};

    uint32_t next_four_bytes = 0;


    set_buffer(buffer_data);


    // first cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    auto cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 0);
    increment_index_in_buffer(1);


    // second cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 1);
    increment_index_in_buffer(1);

    // third cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'b');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 2);
    increment_index_in_buffer(1);


    // third cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'c');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 3);
    increment_index_in_buffer(1);

    // forth cycle first match of 'a a b c'
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 4);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 0);
    EXPECT_EQ(cyclic_arr.array[1], 4);
    increment_index_in_buffer(4);

    // fith cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'd');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 8);
    increment_index_in_buffer(1);

    // sixth cycle second match 'a a b c a'
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 5);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 0);
    EXPECT_EQ(cyclic_arr.array[1], 4);
    EXPECT_EQ(cyclic_arr.array[2], 9);
    increment_index_in_buffer(5);


    // eighth cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'e');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    EXPECT_FALSE(is_key_in_dict(next_four_bytes));
}

TEST_F(LempelZivTest, TestFindWindow_OverlapFuture)
{
    std::vector<uint8_t> buffer_data =
        {'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b', 'a', 'b'};

    uint32_t next_four_bytes = 0;


    set_buffer(buffer_data);


    // first cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'a');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    auto cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 0);
    increment_index_in_buffer(1);

    // second cycle no match
    EXPECT_EQ(find_window(), false);
    EXPECT_EQ(get_literal(), 'b');
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 1);
    increment_index_in_buffer(1);

    // third cycle found overlapping match ' a b a b a b a b a b a b '
    EXPECT_EQ(find_window(), true);
    EXPECT_EQ(get_window_size(), 12);
    EXPECT_EQ(get_start_index(), 0);
    std::memcpy(&next_four_bytes, buffer_data.data() + get_current_index_in_buffer(), 4);
    cyclic_arr = get_array(next_four_bytes);
    EXPECT_EQ(cyclic_arr.array[0], 0);
    EXPECT_EQ(cyclic_arr.array[1], 2);
    increment_index_in_buffer(12);
}

TEST_F(LempelZivTest, TestCyclic_Array_Overflow)
{
    std::vector<uint8_t> buffer_data;
    for (int i = 0; i < 33; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            buffer_data.push_back(0);
        }
        buffer_data.push_back(i + 1);
    }
    set_buffer(buffer_data);

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
        EXPECT_EQ(cyclic_arr.array[i % 32], 5 * i);
    }
}


TEST_F(LempelZivTest, TestBasicCompression)
{
    std::vector<uint8_t> buffer_data =
    {
        'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'c', 'd', 'c', 'd', 'c', 'd', 'd',
        'a', 'b', 'c', 'e', 'a', 'b', 'c', 'e', 'd', 'a', 'a', 'b', 'a', 'a', 'c', 'f', 'g'
    };

    std::vector<uint64_t> expected_coded_vec = {
        'a', 'a', 'b', 0, 5, 'c', 'd', 8, 4, 'd', 'a', 'b', 'c', 'e',
        15, 4, 'd', 3, 6, 'f', 'g'
    };
    std::vector<uint64_t> expected_bit_map = {0b11010111110110111};

    std::string file_path = "basicCompressionTest.bin";

    std::ofstream file(file_path);

    ASSERT_TRUE(file.is_open()) << "Failed to create test file: " << file_path;

    file.write(reinterpret_cast<const char*>(buffer_data.data()), buffer_data.size());
    file.close();

    compressor.compress(file_path);

    auto& res_coded_vec = get_coded_vec();
    auto& res_bit_map = get_bit_map();

    EXPECT_EQ(expected_coded_vec.size(), res_coded_vec.size());
    for (int i = 0; i < expected_coded_vec.size(); i++)
    {
        EXPECT_EQ(expected_coded_vec[i], res_coded_vec[i]);
    }

    EXPECT_EQ(expected_bit_map.size(), res_bit_map.size());
    for (int i = 0; i < res_bit_map.size(); i++)
    {
        EXPECT_EQ(expected_bit_map[i], res_bit_map[i]);
    }
}

TEST_F(LempelZivTest, TestBasicDecompression)
{
    std::vector<uint64_t> coded_vec = {
        'a', 'a', 'b', 0, 5, 'c', 'd', 8, 4, 'd', 'a', 'b', 'c', 'e',
        15, 4, 'd', 3, 6, 'f', 'g'
    };
    std::vector<uint64_t> bit_map = {0b11010111110110111};

    std::vector<uint8_t> expected_data =
    {
        'a', 'a', 'b', 'a', 'a', 'b', 'a', 'a', 'c', 'd', 'c', 'd', 'c', 'd', 'd',
        'a', 'b', 'c', 'e', 'a', 'b', 'c', 'e', 'd', 'a', 'a', 'b', 'a', 'a', 'c', 'f', 'g'
    };


    set_coded_vec(coded_vec);
    set_bit_map(bit_map);

    std::string file_path = "basicDecompressionTest.bin";
    compressor.decompress(file_path);

    std::ifstream file(file_path, std::ios::binary);
    ASSERT_TRUE(file.is_open()) << "Failed to create test file: " << file_path;


    EXPECT_EQ(std::filesystem::file_size(std::filesystem::path(file_path)), 32);


    std::array<uint8_t, 32> res_data;

    file.read(reinterpret_cast<char*>(res_data.data()), 32);
    file.close();

    // how to add a test that the file doesnt have anything else ? that is it is only 31 bytes?

    for (int i = 0; i < 32; i++)
    {
        EXPECT_EQ(res_data[i], expected_data[i]);
    }
}

TEST_F(LempelZivTest, TestOnRealFilesCompressionAndDecompression)
{
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
        compressor.clear();

        compressor.compress(file_name);
        compressor.decompress(res_file_name);


        EXPECT_EQ(std::filesystem::file_size(file_name) , std::filesystem::file_size(res_file_name));

        binary_io::FileReader file(file_name);

        binary_io::FileReader res_file(res_file_name);

        while (file.slide_window() && res_file.slide_window())
        {
            auto file_buffer = file.get_buffer();
            auto res_file_buffer = res_file.get_buffer();

            for(size_t j = 0 ; j < file.get_num_bytes_read(); j++)
            {
                EXPECT_EQ((*file_buffer)[j], (*res_file_buffer)[j]);
            }
        }


    }
}
