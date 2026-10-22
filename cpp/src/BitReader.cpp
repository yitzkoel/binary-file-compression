//
// Created by yitzk on 9/17/2026.
//

#include "BitReader.h"


BitReader::BitReader(uint8_t* buffer, size_t buffer_size):
    buffer(buffer), buffer_iter(buffer), offset_(0), buffer_size(buffer_size)
{
}


void BitReader::set_index(size_t index)
{
    buffer_iter = buffer + index;
}

void BitReader::set_offset(uint8_t offset)
{
    offset_ = offset;
}