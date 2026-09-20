//
// Created by yitzk on 9/17/2026.
//

#ifndef BITREADER_H
#define BITREADER_H
#include <cstdint>
#include <cstddef>


class BitReader {
public:
    BitReader(uint8_t* buffer);

    inline uint64_t read_bits_from_buffer(uint8_t num_bits);

    inline void advance_buffer(uint8_t num_bits);

    uint8_t* get_iter();

    void set_index(size_t index);

    void set_offset(uint8_t offset);

    void set_safe_end(size_t num_future_byts);

    inline void cycle_buffer(uint16_t num_next_bytes_to_recycle);

    inline bool safe_read();
private:



    uint8_t* buffer;
    uint8_t* buffer_iter;
    uint8_t offset_;

    const uint8_t* safe_end;

};



#endif //BITREADER_H
