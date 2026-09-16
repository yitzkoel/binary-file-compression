//
// Created by yitzk on 8/7/2026.
//

#ifndef LEMPEL_ZIV_ALGO_H
#define LEMPEL_ZIV_ALGO_H
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cassert>

#include "hashTable.h"
#include "binary_io.h"


class Lempel_ziv_algo {
public:
    void compress(const std::string& file_path);

    void decompress(const std::string& file_path);

    void clear();


private:
    /**
     *  This method gets a index_to_start_searching that is in the past(that is an index smaller than index_in_buffer)
     *  and a max window size to look ahead (since we dont want to overflow the buffer), and gives us the largest window
     *  that from index_to_start_searching that match a window from index_in_buffer.
     *
     * @param index_to_start_searching the index in the past to start comparing to the current index_in_buffer
     * @param max_window_size the max possible window size to look in the future
     * @return the length of the largest window match starting at index_to_start_searching
     */
    uint64_t find_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size);

    /**
     * serches in the past of the buffer for a window that matches a window in the future.
     * If a window is found then the parameters of that window(start index and window length) are saved in class fields.
     * @return true if a window was found
     */
    bool find_window();


    std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> buffer = nullptr;
    std::vector<uint32_t> coded_vec;
    char LEN_WORD = 64;

    // the hash map
    hashTable hash_map = hashTable(power_of_two_size_of_hash_table);
    static const  int power_of_two_size_of_hash_table = 18;


    uint64_t num_bytes_read = 0;
    uint64_t index_in_buffer = 0 ;

    uint64_t start_window_index = 0;
    uint16_t len_window = 0;
    uint64_t literal = 0;

    inline static const uint64_t MAX_WINDOW_SIZE = (2<<11)+ 22;
    inline static const uint16_t WINDOW_OFFSET = 256;

    friend class LempelZivTest;
};

// ==========================================
// TODO: PERFORMANCE OPTIMIZATIONS
// ==========================================

// TODO (Performance - Memory Allocation): Pre-allocate memory for 'coded_vec' and 'bit_map' inside compress().
// Use 'reserve()' based on the chunk size to prevent costly dynamic reallocations (std::vector growing) inside the hot loop.

// TODO (Performance - CPU Instructions): Optimize 'find_max_window_from_given_index()'.
// Replace the while-loop byte-comparison with a 64-bit XOR operation and '__builtin_ctzll' (count trailing zeros).
// This allows finding the exact match length in 3 CPU instructions without branching.

#endif //LEMPEL_ZIV_ALGO_H
