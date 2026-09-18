//
// Created by yitzk on 9/17/2026.
//

#include "BitReader.h"

#include <cstring>

BitReader::BitReader(uint8_t* buffer):
    buffer(buffer), buffer_iter(buffer), offset_(0), safe_end(nullptr)
{
}

uint64_t BitReader::read_bits_from_buffer(uint8_t num_bits)
{
    uint64_t window = *(uint64_t*)buffer_iter; // get a window of size 64 bytes from the

    window = window >> offset_;

    return  window & ((1U << num_bits) - 1);
}

uint8_t* BitReader::get_iter()
{
    return buffer_iter;
}

void BitReader::set_index(size_t index)
{
    buffer_iter = buffer + index;
}

void BitReader::set_offset(uint8_t offset)
{
    offset_ = offset;
}

void BitReader::set_safe_end(size_t num_future_byts)
{
    safe_end = buffer + num_future_byts;
}

void BitReader::cycle_buffer(uint16_t num_next_bytes_to_recycle)
{
    // copy the last 8 bytes to the front of the buffer
    memcpy(buffer, safe_end, num_next_bytes_to_recycle);

    // have buffer iter point to the first byte in buffer with data that was not yet consumed
    buffer_iter = buffer + (buffer_iter - safe_end);
}

bool BitReader::safe_read()
{
    return buffer_iter < safe_end;
}

void BitReader::advance_buffer(uint8_t num_bits)

{
    offset_ += num_bits;
    buffer_iter += offset_ >> 3; // devide offset by 8 and add to the iter.
    offset_ = offset_ &= 7; // modolo 8
}
