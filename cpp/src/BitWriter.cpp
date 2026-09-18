//
// Created by yitzk on 9/17/2026.
//

#include "BitWriter.h"

#include <cstring>

BitWriter::BitWriter(uint8_t* buffer):buffer(buffer), iter(buffer), offset(0)
{
}

void BitWriter::write_bits_to_buffer(uint64_t src, uint8_t num_bits)
{
    uint32_t window = 0;

    memcpy(&window, iter, 4);

    window &= (1ULL << offset) - 1;

    src = src << offset;

    window = window | src;

    std::memcpy(iter, &window, sizeof(window));

    advance_buffer(num_bits);

}

void BitWriter::write_byte_array(const uint8_t* src, size_t num_bytes)
{
    for(size_t i = 0; i < num_bytes; i++, iter++)
    {
        *iter = src[i];
    }
}

size_t BitWriter::num_bytes_writen_to_buffer()
{
    return iter - buffer;
}

void BitWriter::reset()
{
    iter = buffer;
    offset = 0;
}

void BitWriter::advance_buffer(uint8_t num_bits)
{
    offset += num_bits;
    iter += offset >> 3; // devide offset by 8 and add to the iter.
    offset = offset &= 7; // modolo 8
}
