//
// Created by yitzk on 8/7/2026.
//

#include "lempel_ziv_algo.h"

Lempel_ziv_algo::Lempel_ziv_algo(std::string& file_path) :
    input_file(file_path),
    buffer(input_file.get_buffer()),
    num_bytes_read(input_file.get_num_bytes_read()),
    bit_map(1, 0)
{
}

void Lempel_ziv_algo::compress()
{
    while (input_file.slide_window())
    {
        num_bytes_read += input_file.get_num_bytes_read();
        index_in_buffer = 0;
        hash_map.clear();

        while (index_in_buffer < num_bytes_read)
        {
            if (find_window())
            {
                // handle window found
                add_window_to_vec();

                // update index in the buffer
                index_in_buffer += len_window;
            }

            else
            {
                // handle addind literal
                add_literal_to_vec();

                // update index in the buffer
                index_in_buffer++;
            }
        }
    }
}

void Lempel_ziv_algo::decompress(std::string& file_path)
{
    binary_io::FileWriter output_file(file_path);

    uint64_t mask = 1; // used to read the current bit in the bitmap
    uint64_t index_in_bitmap = 0;
    index_in_buffer = 0;
    uint64_t index_in_coded_vec = 0;

    // read the coded vec
    while (index_in_coded_vec < coded_vec.size())
    {

        // case 1: the current bit is 0:  the cuurent value in the coded vector is a literal
        if ((bit_map[index_in_bitmap] & mask) == 0)
        {
            // write into the buffer the current literal in coded vec
            buffer[index_in_buffer] = coded_vec[index_in_coded_vec];

            // update indexs
            index_in_buffer++;
            index_in_coded_vec++;
        }
        // case 2: the current bit is 0: the next two values in the coded vec code the the past index and length of window
        else
        {
            // read the next two values in the coded vec that code the start index of the window and the window length
            start_window_index = coded_vec[index_in_coded_vec];
            index_in_coded_vec++;
            len_window = coded_vec[index_in_coded_vec];
            index_in_coded_vec++;
            // TODO safty check did we accedently passed the vec size or the bitmap and so on?

            // case 1: the window is all in the past safe to use memcpy and faster
            if(start_window_index + len_window <= index_in_buffer) memcpy(&buffer[index_in_buffer], &buffer[start_window_index], len_window);

            // case 2: the window overlap the place we write into therefor we need to copy byte by byte to avoid corruption of the data.
            else
            {
                for(uint64_t i = 0; i < len_window; i++)
                {
                    buffer[index_in_buffer + i] = buffer[start_window_index + i];
                }
            }

            index_in_buffer += len_window;
        }

        // update the mask to the next bit
        mask = mask << 1;

        // if we did 64 shift operation we need to update the mask back to the first bit and advance the bitmap index
        if (mask == 0)
        {
            mask = 1;
            index_in_bitmap++;
        }

        if (index_in_buffer == buffer.size())
        {
           output_file.flush_buffer_to_file(buffer, buffer.size() );
            index_in_buffer = 0;
        }
    }

    output_file.flush_buffer_to_file(buffer, index_in_buffer);
}


bool Lempel_ziv_algo::find_window()
{
    // calculate max window size to look for
    uint64_t max_window_size = num_bytes_read - index_in_buffer;

    // calculate the hash map for the 4 next bytes in the buffer
    uint32_t next_four_bytes = 0;
    std::memcpy(&next_four_bytes, &buffer[index_in_buffer], 4);

    // get iterator to the cyclic map of all previous potential matches
    auto iter = hash_map.find(next_four_bytes);

    // if we found that there are previos potential matches we start to search through them to find the best one
    if (iter != hash_map.end())
    {
        // get the actual cyclic array
        auto& array = iter->second;

        // find till what index does the cyclic array hold valid past indexes.
        uint8_t max_index = array.index_in_array;
        if (array.array_full) max_index = cyclicArray::NUM_ELEMENTS_IN_ARRAY;

        uint32_t max_index_past_match = array.array[0]; // init to first match past index
        uint64_t max_matching_window_size = 4;
        for (int i = 0; i < max_index; i++)
        {
            uint32_t cur_index_past_match = array.array[i];
            uint64_t cur_max_matching_window_size = find_max_window_from_given_index(
                cur_index_past_match, max_window_size);

            if (max_matching_window_size < cur_max_matching_window_size)
            {
                max_index_past_match = cur_index_past_match;
                max_matching_window_size = cur_max_matching_window_size;
            }
        }
        // add tho the cyclic array the current index in the buffer since it also starts with those 4 bytes
        array.add_elem(index_in_buffer);

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
        hash_map[next_four_bytes];

        // add the new entry hash map cyclic array this current index.
        hash_map[next_four_bytes].add_elem(index_in_buffer);
        literal = buffer[index_in_buffer];

        // return failed to find a window
        return false;
    }
}

uint64_t Lempel_ziv_algo::find_max_window_from_given_index(uint32_t index_to_start_searching, uint64_t max_window_size)
{
    // The current max window we manged to find
    uint64_t max_window_len = 0;

    // the current lenght of comparison
    uint64_t len = 32;

    // if we have in the future(the unseen buffer) enoght bytes to search (that is len bytes)
    // we will attempt comparint the windows 32 bytes at a time
    if (max_window_size >= 32)
    {
        while (memcmp(&buffer[index_in_buffer + max_window_len],
                           &buffer[index_to_start_searching + max_window_len],
                           len) != 0)
        {
            max_window_len += 32;

            // if the remaining futer bytes are smaller than len we cant compare the next len bytes
            if (max_window_len - max_window_size < len) break;
        }
    }

    // We have at this point at most another 31 bytes ahead to compare
    len = len >> 1; // len = 16

    // we keep comparing till the shift oper turns len to 0
    while (len > 0)
    {
        // if the remaining futer bytes are smaller than len we cant compare the next len bytes
        if (max_window_len - max_window_size < len) len = len >> 1;
        if (memcmp(&buffer[index_in_buffer + max_window_len], &buffer[index_to_start_searching + max_window_len],
                        len) != 0)
        {
            max_window_len += len;
        }
        len = len >> 1;
    }

    return max_window_len;
}

void Lempel_ziv_algo::add_window_to_vec()
{
    coded_vec.push_back(start_window_index);
    coded_vec.push_back(len_window);
    bit_map_mask = bit_map_mask << 1;

    update_bit_map();
}

void Lempel_ziv_algo::add_literal_to_vec()
{
    coded_vec.push_back(literal);

    // turn on the current bit in the bitmap
    bit_map[bit_map.size()] = bit_map[bit_map.size()] & bit_map_mask;

    bit_map_mask = bit_map_mask << 1;

    update_bit_map();
}

void Lempel_ziv_algo::update_bit_map()
{
    if (bit_map_mask == 1 << 63)
    {
        bit_map_mask = 1;
        bit_map.push_back(0);
    }
}

uint64_t Lempel_ziv_algo::min(uint64_t val1, uint64_t val2)
{
    return val1 < val2 ? val1 : val2;
}
