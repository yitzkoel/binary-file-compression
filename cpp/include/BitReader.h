//
// Created by yitzk on 9/17/2026.
//

#ifndef BITREADER_H
#define BITREADER_H
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>

/**
 * @brief A class that lets you read bits from a buffer.
 *
 */
class BitReader
{
public:
    /**
     * Constructor
     * @param buffer The buffer to write into.
     */
    explicit BitReader(uint8_t* buffer, size_t buffer_size);

    /**
     * this function lets you look at the next 'num bits' in the buffer.
     * @param num_bits number of bits to peak at.
     * @return the bits requested
     */
    uint32_t peak_bits(uint8_t num_bits)
    {
        // make sure that the number of bits read is supported by the window var.
        assert(num_bits <= 32 && "The bit reader supports reading at most 32 bits in one read");

        // get a window of size 64 bytes from the buffer
        uint64_t window = 0;
        memcpy(&window, buffer_iter, 8);

        // remove the already read offset from the window
        window = window >> offset_;

        // create a mask to select the first 'num_bits' bits from the window
        window &= ((1ULL << num_bits) - 1);

        return window;
    }

    /**
     * this function advances the buffer reader 'num_bits' bits.
     * @param num_bits the number of bits to advance the buffer.
     */
    void advance_buffer(uint8_t num_bits)
    {
        // addvande the offset num bits
        offset_ += num_bits;
        // add to the buffer the number of bytes that offset holds
        buffer_iter += offset_ >> 3; // devide offset by 8 and add to the iter.
        // recalculate the offset to be the offset of the current byte.
        offset_ &= 7; // modolo 8 to get the real offset
    }

    /**
     * Getter of pointer the the current byte in the buffer.
     * @return uint8_t pointer to the current byte in the buffer
     */
    uint8_t* get_iter()
    {
        return buffer_iter;
    }

    uint8_t get_offset() const
    {
        return offset_;
    }

    uint8_t* get_buffer() const
    {
        return buffer;
    }

    /**
     * this function sets the index of the uint8_t buffer to 'index'.
     * @param index the index in the buffer to set the bit reader to
     */
    void set_index(size_t index);

    /**
     * Sets the offset at the current byte in the buffer.
     * @param offset the offset in the current index in the buffer (a value between 0 and 7)
     */
    void set_offset(uint8_t offset);


private:
    //------------------------------------------------
    // FIELDS
    //------------------------------------------------

    //the buffer ptr to the buffer
    uint8_t* buffer;
    // the ptr to the current byte in the buffer
    uint8_t* buffer_iter;
    // the offset in the cuurent byte (a value between 0 and 7)
    uint8_t offset_;

    size_t buffer_size;


    // added for testing
    friend class TestBitReader;
};


#endif //BITREADER_H
