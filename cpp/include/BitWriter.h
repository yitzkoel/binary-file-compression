//
// Created by yitzk on 9/17/2026.
//

#ifndef BITWRITER_H
#define BITWRITER_H
#include <cstdint>


#include <cstdint>


class BitWriter
{
public:
    explicit BitWriter(uint8_t* buffer);
    inline void write_bits_to_buffer(uint64_t src, uint8_t num_bits);

    inline void write_byte_array(const uint8_t* src, size_t num_bytes);

    inline size_t num_bytes_writen_to_buffer();

    void reset();

private:
    inline void advance_buffer(uint8_t num_bits);

    uint8_t* buffer;
    uint8_t* iter;
    uint8_t offset;
};


#endif //BITWRITER_H
