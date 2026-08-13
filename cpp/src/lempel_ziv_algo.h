//
// Created by yitzk on 8/7/2026.
//

#ifndef LEMPEL_ZIV_ALGO_H
#define LEMPEL_ZIV_ALGO_H
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>

#include "binary_io.h"

struct cyclicArray
{
    const static  size_t NUM_ELEMENTS_IN_ARRAY = 32;
    std::array<uint32_t,NUM_ELEMENTS_IN_ARRAY> array;
    uint8_t index_in_array = 0;
    bool array_full = false;

    void add_elem(uint32_t new_elem)
    {
        array[index_in_array] = new_elem;
        if(index_in_array >= NUM_ELEMENTS_IN_ARRAY)
        {
            index_in_array = 0;
            array_full = true;
        }
    }
};


class Lempel_ziv_algo {
public:
    explicit Lempel_ziv_algo(std::string& file_path);

    void compress();

    void decompress(std::string& file_path);


private:
    uint64_t find_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size);
    bool find_window();
    void add_window_to_vec();
    void add_literal_to_vec();
    void update_bit_map();
    static uint64_t min(uint64_t val1, uint64_t val2);

    std::array<uint8_t,BUFFER_SIZE>& buffer;
    std::vector<uint64_t> coded_vec;
    std::vector<uint64_t> bit_map;
    uint64_t bit_map_mask = 0;
    char LEN_WORD = 64;

    // the hash map
    std::unordered_map<uint32_t,cyclicArray> hash_map;

    binary_io::FileReader input_file;

    uint64_t num_bytes_read{};
    uint64_t index_in_buffer = 0 ;

    uint64_t start_window_index = 0;
    uint64_t len_window = 0;
    uint64_t literal = 0;
};



#endif //LEMPEL_ZIV_ALGO_H
