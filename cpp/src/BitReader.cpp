//
// Created by yitzk on 9/17/2026.
//

#include "BitReader.h"


BitReader::BitReader(uint8_t* buffer):
    buffer(buffer), buffer_iter(buffer), offset_(0), safe_end(nullptr)
{
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

void BitReader::alighn_reader_to_byte()
{
    if(offset_ > 0) buffer_iter +=1 ;
    offset_ = 0;
}

