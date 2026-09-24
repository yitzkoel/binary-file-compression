//
// Created by yitzk on 9/22/2026.
//

#include <../include/BitReader.h>
#include <gtest/gtest.h>
#include <cstring> // For std::memset and std::memcpy
#include <array>
#include <memory>

#define BUFFER_SIZE (1<<22) // 4MB

class TestBitReader : public ::testing::Test
{
protected:
    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buffer =
        std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();

    uint8_t* buffer_ptr;

    BitReader bit_reader;

    // Updated Constructor: passing BUFFER_SIZE to the BitReader
    TestBitReader(): bit_reader(buffer->data(), BUFFER_SIZE), buffer_ptr(buffer->data())
    {
    }

    [[nodiscard]] uint8_t get_offset() const
    {
        return bit_reader.get_offset();
    }
};

TEST_F(TestBitReader, AssertOnTooManyBits)
{
    std::cout << "[INFO] Death Test: Prevent reading > 32 bits at once.\n";
    // Verifies that the BitReader correctly asserts and crashes if instructed
    // to read more than 32 bits, which is outside its supported bounds.
    EXPECT_DEATH(
        bit_reader.peak_bits(33),
        "The bit reader supports reading at most 32 bits in one read");

    // Verify normal execution doesn't crash
    bit_reader.peak_bits(32);

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestReadingBits_Test1)
{
    std::cout << "[INFO] Basic Test: Read a single byte from the buffer.\n";
    // Sets the first byte to 0xA and verifies that reading 8 bits
    // correctly retrieves this exact value.
    *buffer_ptr = 0xA;

    uint64_t res = bit_reader.peak_bits(8);

    EXPECT_EQ(res, 0xA);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestReadingBits_Test2)
{
    std::cout << "[INFO] Use Case Test: Read 13 bits from the buffer.\n";
    // Fills two bytes with alternating bits (0xAA) and reads exactly 13 bits.
    // Verifies the underlying bitwise masking logic works for non-byte multiples.
    *buffer_ptr = 0xAA;
    *(buffer_ptr + 1) = 0xAA;

    uint8_t num_bits = 13;
    uint64_t res = bit_reader.peak_bits(num_bits);

    EXPECT_EQ(res, 0x0AAA);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestReadingBits_Test3)
{
    std::cout << "[INFO] Edge Case Test: Read 0 bits from the buffer.\n";
    // Validates that requesting 0 bits safely returns 0 without causing
    // undefined behavior (like shifting by 64).
    *buffer_ptr = 0xAA;
    *(buffer_ptr + 1) = 0xAA;

    uint8_t num_bits = 0;
    uint64_t res = bit_reader.peak_bits(num_bits);

    EXPECT_EQ(res, 0);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestReadingBits_Test4)
{
    std::cout << "[INFO] Edge Case Test: Read exactly 32 bits (Max value).\n";

    // Part 1: Basic test with offset = 0
    // Using memcpy to avoid Strict Aliasing violations on direct pointer casting
    uint32_t val_32 = 0xAAAAAAAA;
    std::memcpy(buffer_ptr, &val_32, sizeof(val_32));

    uint8_t num_bits = 32;
    uint32_t res = bit_reader.peak_bits(num_bits);
    EXPECT_EQ(res, 0xAAAAAAAA);

    // Part 2: Test reading 32 bits with an internal offset
    uint8_t offset = 3;
    bit_reader.set_offset(offset);
    EXPECT_EQ(offset, get_offset());

    // Safely write a 64-bit value to memory to simulate continuous bits.
    // The value 0x56789ABCD spans 36 bits (5 bytes).
    uint64_t val_64 = 0x56789ABCD;
    std::memcpy(buffer_ptr, &val_64, sizeof(val_64));

    // Shifting right by 3 bits (offset) should yield 0xACF13579
    res = bit_reader.peak_bits(num_bits);
    EXPECT_EQ(res, 0xACF13579);

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestAdvanceBuffer_Test1)
{
    std::cout << "[INFO] Basic Test: Advance buffer by 3 bits.\n";
    // Verifies that advancing by a small amount updates the offset correctly
    // without crossing a byte boundary.
    *buffer_ptr = 0xAA;
    uint8_t num_bits = 3;

    bit_reader.advance_buffer(num_bits);

    EXPECT_EQ(num_bits, get_offset());
    EXPECT_EQ(buffer_ptr, bit_reader.get_iter());
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestAdvanceBuffer_Test2)
{
    std::cout << "[INFO] Use Case Test: Advance buffer by 15 bits.\n";
    // Verifies that advancing by more than 8 bits correctly increments
    // the byte iterator and leaves the remainder in the offset.
    *buffer_ptr = 0xAA;
    uint8_t num_bits = 15;

    bit_reader.advance_buffer(num_bits);

    EXPECT_EQ(num_bits % 8, get_offset());
    EXPECT_EQ(buffer_ptr + (num_bits / 8), bit_reader.get_iter());
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestAdvanceBuffer_Test3)
{
    std::cout << "[INFO] Edge Case Test: Advance buffer by 0 bits.\n";
    // Ensures that advancing by 0 has no side effects on iterators or offsets.
    *buffer_ptr = 0xAA;
    uint8_t num_bits = 0;

    bit_reader.advance_buffer(num_bits);

    EXPECT_EQ(num_bits % 8, get_offset());
    EXPECT_EQ(buffer_ptr + (num_bits / 8), bit_reader.get_iter());
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestAdvanceBuffer_Test4)
{
    std::cout << "[INFO] Edge Case Test: Advance buffer by 255 bits (Max uint8).\n";
    // Tests the upper boundary of uint8_t to ensure large advances
    // are calculated cleanly without integer overflow.
    *buffer_ptr = 0xAA;
    uint8_t num_bits = 255;

    bit_reader.advance_buffer(num_bits);

    EXPECT_EQ(num_bits % 8, get_offset());
    EXPECT_EQ(buffer_ptr + (num_bits / 8), bit_reader.get_iter());
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestAdvanceBuffer_Test5)
{
    std::cout << "[INFO] Sequence Test: Advance buffer incrementally with varied steps.\n";
    // Simulates a real-world scenario by advancing the buffer multiple times
    // in unpredictable intervals, validating accumulated state tracking.
    std::memset(buffer_ptr, 0xAA, 8); // Safely fill 8 bytes

    std::vector<uint8_t> num_bits_vec = {3, 4, 7, 8, 13, 0, 1};
    uint8_t num_bits_advanced_so_far = 0;

    for (auto num_bits : num_bits_vec)
    {
        bit_reader.advance_buffer(num_bits);
        num_bits_advanced_so_far += num_bits;

        EXPECT_EQ(num_bits_advanced_so_far % 8, get_offset());
        EXPECT_EQ(buffer_ptr + (num_bits_advanced_so_far / 8), bit_reader.get_iter());
    }

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestOverallPerformance)
{
    std::cout << "[INFO] Integration Test: Read and advance buffer continuously.\n";
    // End-to-end simulation. Fills the buffer with 1s and reads chunks of varying sizes.
    // Validates that state (offset and iter) and read values remain consistent throughout.

    std::memset(buffer_ptr, 0xFF, 16);

    std::vector<uint8_t> num_bits_to_read_vec = {3, 4, 7, 8, 13, 0, 1, 15, 22, 1, 1, 1, 3, 4, 15};

    uint8_t num_bits_advanced_so_far = 0;
    uint64_t expected_res = 0xFFFFFFFFFFFFFFFF;

    for (auto num_bits : num_bits_to_read_vec)
    {
        uint32_t res = bit_reader.peak_bits(num_bits);
        bit_reader.advance_buffer(num_bits);
        num_bits_advanced_so_far += num_bits;

        // Check internal state
        EXPECT_EQ(num_bits_advanced_so_far % 8, get_offset());
        EXPECT_EQ(buffer_ptr + (num_bits_advanced_so_far / 8), bit_reader.get_iter());

        // Check reading accuracy (avoids 64-bit shift UB when num_bits is 0)
        if (num_bits == 0)
            EXPECT_EQ(res, 0);
        else
            EXPECT_EQ(res, expected_res >> (64 - num_bits));
    }
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestUnalignedReadCrash)
{
    std::cout << "[INFO] Sanitizer Test: Verifies safe unaligned memory access.\n";
    // Forces the iterator to an odd address (not divisible by 8) and performs
    // a subsequent read. If the code uses unsafe pointer casting, UBSAN/ASAN
    // will intentionally crash this test. Safe memcpy implementations will pass.
    std::memset(buffer_ptr, 0xAA, 16);

    bit_reader.peak_bits(8);
    uint32_t res = bit_reader.peak_bits(15);

    EXPECT_GT(res, 0);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitReader, TestCyclicBuffer_Test1)
{
    std::cout << "[INFO] Buffer Management Test: Cyclic buffer wrap-around.\n";
    // Simulates reading near the physical end of a cyclic buffer.
    // Validates that 'cycle_buffer' properly copies leftover bytes to the start
    // and correctly repositions the iterator to maintain logical continuity.
    size_t buffer_size = 20;
    uint8_t buffer[buffer_size];

    // Initialize with the new API including buffer_size
    BitReader bit_reader_cyclic(buffer, buffer_size);

    // We want the safe end to be at index 12.
    // Thus, it is 8 bytes from the buffer's end (20 - 8 = 12).
    bit_reader_cyclic.set_safe_end(8);

    std::memset(buffer, 0xAA, 12);
    std::memset(buffer + 12, 0xFF, 8); // Data that triggers wrap-around

    // Position iterator inside the "unsafe" wrap-around zone (index 13)
    bit_reader_cyclic.set_index(13);
    bit_reader_cyclic.set_offset(3);

    // Perform the cycle logic using internal size state
    bit_reader_cyclic.cycle_buffer();

    // Verify iterator wraps to the correct relative position.
    // Old index 13 was 1 byte past safe_end (12). So new index should be 1.
    EXPECT_EQ(buffer + 1, bit_reader_cyclic.get_iter());

    // Verify reading continuous data post-wrap works flawlessly
    uint32_t expected_res = 0xFFFFFFFF;
    uint32_t res = bit_reader_cyclic.peak_bits(32);
    EXPECT_EQ(res, expected_res);

    std::cout << "*************** Finish test ***********\n\n";
}