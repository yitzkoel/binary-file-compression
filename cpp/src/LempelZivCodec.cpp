//
// Created by yitzk on 8/7/2026.
//

#include "../include/LempelZivCodec.h"

CodedVec LempelZivCodec::compress(uint8_t* buffer_ptr, size_t buffer_size)
{
    CodedVec code_vec;

    // reserve space in the vector to prevent allocation in the loop below
    code_vec.reserve(buffer_size / 2);

    buffer = buffer_ptr;
    num_bytes_in_buffer = buffer_size;
    hash_map.clear();
    index_in_buffer = 0;

    while (index_in_buffer < num_bytes_in_buffer)
    {
        if (find_window())
        {
            // add window and distance index

            // the window length with the offset
            code_vec.push_back(len_window + WINDOW_OFFSET);
            // the distance to the window from the current index
            code_vec.push_back(index_in_buffer - start_window_index);

            // update index in the buffer
            index_in_buffer += len_window;
        }

        else
        {
            // addind literal
            code_vec.push_back(literal);

            // update index in the buffer
            index_in_buffer++;
        }
    }
    code_vec.push_back(EOF_SYMBOL);
    return code_vec;
}

uint64_t LempelZivCodec::decompress(uint8_t* buffer_ptr, size_t buffer_size, CodedVec& code_vec)
{
    index_in_buffer = 0;
    buffer = buffer_ptr;
    uint32_t val = 0;

    for (size_t index_in_coded_vec = 0; index_in_coded_vec < code_vec.size();)
    {
        // read the coded vec
        val = code_vec[index_in_coded_vec];
        index_in_coded_vec++;

        // case 1:  the cuurent value in the coded vector is a literal
        if (val <= 256)
        {
            // we reached the end of the vector
            if (val == EOF_SYMBOL) break;

            // write into the buffer the current literal in coded vec
            buffer[index_in_buffer] = val;

            // update index
            index_in_buffer++;
        }
        // case 2: the next two values in the coded vec code the the past index and length of window
        else
        {
            // read the next two values in the coded vec that code the start index of the window and the window length
            len_window = val - WINDOW_OFFSET;
            start_window_index = index_in_buffer - code_vec[index_in_coded_vec];
            index_in_coded_vec++;

            //--------------------
            // SAFTY CHECKS
            //--------------------

            // check if the buffer will overflow
            if (index_in_buffer + len_window > buffer_size) [[unlikely]] {
                throw std::out_of_range("CRITICAL: Buffer overflow detected during match decoding.");
            }

            // check if the distance from the point where the window start is even possible
            // (if it is true then "index_in_buffer - code_vec[index_in_coded_vec]  < 0" wich is impossible.
            if (code_vec[index_in_coded_vec - 1] > index_in_buffer) [[unlikely]]{
                throw std::runtime_error("CRITICAL: Corrupted LZSS vector -distance code matched a point before buffer start.");
            }
            

            // case 1: the window is all in the past safe to use memcpy and faster
            if (start_window_index + len_window <= index_in_buffer)
                memcpy(
                    buffer + index_in_buffer, buffer + start_window_index, len_window);

            // case 2: the window overlap the place we write into there for we need to copy byte by byte to avoid corruption of the data.
            else
            {
                for (uint64_t i = 0; i < len_window; i++)
                {
                    buffer[index_in_buffer + i] = buffer[start_window_index + i];
                }
            }

            index_in_buffer += len_window;
        }
    }
    //the vector doesnt end with EOF SYMBOL as it should which mean it was corrupted
    if (val != EOF_SYMBOL) [[unlikely]] throw std::runtime_error(
        "CRITICAL: The vector didn't end with EOF symbol must have been corrupted.");

    return index_in_buffer;
}

void LempelZivCodec::clear()
{
    buffer = nullptr;

    // the hash map
    hash_map.clear();

    num_bytes_in_buffer = 0;
    index_in_buffer = 0;

    start_window_index = 0;
    len_window = 0;
    literal = 0;
}


bool LempelZivCodec::find_window()
{
    // calculate max window size to look for
    uint64_t max_window_size = std::min(num_bytes_in_buffer - index_in_buffer, MAX_WINDOW_SIZE);

    // if the potential window size is at most 3 it is not worth the compression
    if (max_window_size < 4)
    {
        literal = buffer[index_in_buffer];
        return false;
    }

    // calculate the hash map for the 4 next bytes in the buffer
    uint32_t next_four_bytes = 0;
    std::memcpy(&next_four_bytes, buffer + index_in_buffer, 4);

    // get iterator to the cyclic map of all previous potential matches
    auto iter = hash_map.find(next_four_bytes);

    // if we found that there are previos potential matches we start to search through them to find the best one
    if (iter != nullptr)
    {
        // get the actual cyclic array
        auto array = iter->array_ptr;

        // find till what index does the cyclic array hold valid past indexes.
        uint8_t max_index = iter->index_in_array;
        if (iter->array_full) max_index = NUM_ELEMENTS_IN_ARRAY;

        uint32_t max_index_past_match = array[0]; // init to first match past index
        uint64_t max_matching_window_size = 4;
        for (int i = 0; i < max_index; i++)
        {
            uint32_t cur_index_past_match = array[i];
            uint64_t cur_max_matching_window_size = find_max_window_from_given_index(
                cur_index_past_match, max_window_size);

            if (max_matching_window_size < cur_max_matching_window_size)
            {
                max_index_past_match = cur_index_past_match;
                max_matching_window_size = cur_max_matching_window_size;
            }
        }
        // add to the cyclic array the current index in the buffer since it also starts with those 4 bytes
        iter->add_elem(index_in_buffer);

        // update the max window found data
        start_window_index = max_index_past_match;
        len_window = max_matching_window_size;

        // return true for found a window in the past
        return true;
    }

    // there is now previous apearnces of a window of size 4 so we will add a new array to the hashmap
    else
    {
        // add new entry to the hash map
        hash_map.add(next_four_bytes);

        // add the new entry hash map cyclic array this current index.
        hash_map.find(next_four_bytes)->add_elem(index_in_buffer);
        literal = buffer[index_in_buffer];

        // return failed to find a window
        return false;
    }
}

uint64_t LempelZivCodec::find_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size)
{
    //TODO can optemise the code after the 32 bit compare with XOR compare of 8 byte numbers to avoid branching commands
    //############# ASSERT DECLERTIONS ###############//
    // make sure the buffer is not null
    assert(buffer != nullptr && "Buffer must be initialized before searching");
    // make sure that index_to_start_searching is actually in the past
    assert(index_to_start_searching < index_in_buffer && "Search index must be in the past");
    // 3make sure that we wont overflow the buffer in the code
    assert(index_in_buffer + max_window_size <= BUFFER_SIZE && "Window check exceeds physical buffer bounds");
    //############# ASSERT DECLERTIONS ###############//


    // The current max window we manged to find
    uint64_t max_window_len = 0;

    // the current lenght of comparison
    size_t len = 32;

    // if we have in the future(the unseen buffer) enoght bytes to search (that is len bytes)
    // we will attempt comparint the windows 32 bytes at a time
    if (max_window_size >= 32)
    {
        while (memcmp(buffer + index_in_buffer + max_window_len,
                      buffer + index_to_start_searching + max_window_len,
                      len) == 0)
        {
            max_window_len += 32;

            // if the remaining futer bytes are smaller than len we cant compare the next len bytes
            if (max_window_size - max_window_len < len) break;
        }
    }

    // We have at this point at most another 31 bytes ahead to compare
    len = len >> 1; // len = 16

    // we keep comparing till the shift oper turns len to 0
    while (len > 0)
    {
        // if the remaining future bytes are smaller than the len we want to compare to
        // Then we cant cant compare the next len bytes(since it will compare overflowing the allowed max window)
        if (max_window_size - max_window_len < len)
        {
            len = len >> 1;
            continue;
        }

        // else we can do the comparison
        if (memcmp(buffer + index_in_buffer + max_window_len,
                   buffer + index_to_start_searching + max_window_len,
                   len) == 0)
        {
            max_window_len += len;
        }
        len = len >> 1;
    }

    return max_window_len;
}
