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
        index_in_array++;
        if(index_in_array >= NUM_ELEMENTS_IN_ARRAY)
        {
            index_in_array = 0;
            array_full = true;
        }
    }
};


class Lempel_ziv_algo {
public:
    explicit Lempel_ziv_algo();

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

    /**
     * Adds the fields 'start_window_index', 'len_window'  to the back of the vector(in that order)
     */
    void add_window_to_vec();

    /**
     * Adds the field 'literal' to the back of the vector
     */
    void add_literal_to_vec();

    /**
     * Resets the field bit_map_mask and adds a new elemnt to the bitmap
     */
    void update_bit_map();


    /**
     * Returns the minimum value of 2 POSITIVE 64 bit integers.
     * @param val1 a positive integer
     * @param val2 a positive integer
     * @return The minimum value of val1, val2
     */
    static uint64_t min(uint64_t val1, uint64_t val2);

    std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> buffer = nullptr;
    std::vector<uint32_t> coded_vec;
    std::vector<uint64_t> bit_map;
    uint64_t bit_map_mask = 1;
    char LEN_WORD = 64;

    // the hash map
    std::unordered_map<uint32_t,cyclicArray> hash_map;
    // TODO buid an actual hash map

    uint64_t num_bytes_read = 0;
    uint64_t index_in_buffer = 0 ;

    uint64_t start_window_index = 0;
    uint64_t len_window = 0;
    uint64_t literal = 0;

    friend class LempelZivTest;
};

// ==========================================
// TODO: PERFORMANCE OPTIMIZATIONS
// ==========================================

// TODO (Performance - Hash Table): Replace 'std::unordered_map' with a custom flat array hash table.
// The standard map uses separate chaining, causing severe CPU cache misses on every single byte processed.
// A flat vector of cyclicArrays (e.g., size 1<<20) using a simple bit-shift hash will drastically improve speed.

// TODO (Performance - Memory Allocation): Pre-allocate memory for 'coded_vec' and 'bit_map' inside compress().
// Use 'reserve()' based on the chunk size to prevent costly dynamic reallocations (std::vector growing) inside the hot loop.

// TODO (Performance - CPU Instructions): Optimize 'find_max_window_from_given_index()'.
// Replace the while-loop byte-comparison with a 64-bit XOR operation and '__builtin_ctzll' (count trailing zeros).
// This allows finding the exact match length in 3 CPU instructions without branching.


// ==========================================
// TODO: ARCHITECTURE & LOGIC
// ==========================================

// TODO (Architecture - Bitmap Flushing): Consider optimizing the bitmap literal writing.
// Instead of turning on bits one by one using '.back()', accumulate bits in a local 64-bit register
// and only flush (push_back) to 'bit_map' when the register is full.

#endif //LEMPEL_ZIV_ALGO_H
