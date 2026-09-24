//
// Created by yitzk on 9/17/2026.
//

#ifndef BITWRITER_H
#define BITWRITER_H
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>


class BitWriter
{
public:
    /**
     * Constructor.
     * @param buffer the buffer to write into
     */
    explicit BitWriter(uint8_t* buffer);

    /**
     * this function allows to write to the current location in the buffer 'num bits' bits from the lsb bits of src.
     * @param src the source to write the bits from (the first lsb 'num_bits')
     * @param num_bits the number of lsb to write into the buffer (cant be more that 32)
     */
    void write_bits_to_buffer(uint32_t src, uint8_t num_bits)
    {
        // TODO maby can optimization this is a bottleneck

        // check that the var num_bits is legal
        assert(num_bits <= 32 && "The bit writer supports writing at most 32 bits at a time");

        // copy a window of 64 bit
        uint64_t window = 0;
        memcpy(&window, iter, 8);

        // zero out the window execpt to the offset that was already writtin (we dont want to ovewrite)
        window &= (1ULL << offset) - 1;

        // get the first 'num_bist' lsb of 'src'
        src &= (1ULL << num_bits) - 1;

        // cast src to 64 bit var to ovoiod overfloww when we shift it
        uint64_t src_64 = src;

        // alighn src with window
        src_64 = src_64 << offset;

        // copy src into window
        window = window | src_64;

        // write window back into the buffer
        std::memcpy(iter, &window, sizeof(window));

        // advance the number of bits read
        advance_buffer(num_bits);
    }

    /**
     * this function lets you write whole arrays into the buffer.
     * Please note:
     * 1. There are no safty checks so the user should check if the array can fit in the buffer safely.
     * 2. This methoed ignores the offset and treates the buffer as an array of uint8_t bytes so if there is data
     *    in the offset of the current byte it will be overwritin by the first element in the 'src' array.
     * @param src the array to write into the buffer.
     * @param num_bytes the number of bytes to write from the array
     */
    void write_byte_array(const uint8_t* src, size_t num_bytes)
    {
        memcpy(iter, src, num_bytes);
        iter += num_bytes;
    }

    /**
     * returns the number of bytes writen to the buffer so far including the byte that we are in the middle of (the
     * offset is not 0).
     * @return the number of bytes writen to the buffer so far.
     */
    size_t num_bytes_writen_to_buffer()
    {
        return iter - buffer + (offset > 0 ? 1 : 0);
    }

    /**
     * resets the bit reader to the begining of the buffer.
     */
    void reset()
    {
        iter = buffer;
        offset = 0;
    }

private:
    //------------------------------
    // HELPER FUNCTIONS
    //------------------------------
    void advance_buffer(uint8_t num_bits)
    {
        offset += num_bits;
        iter += offset >> 3; // devide offset by 8 and add to the iter.
        offset = offset &= 7; // modolo 8
    }

    //------------------------------
    // FIELDS
    //------------------------------
    uint8_t* buffer;
    uint8_t* iter;
    uint8_t offset;


    // added for testing
    friend class TestBitWriter;
};


#endif //BITWRITER_H
