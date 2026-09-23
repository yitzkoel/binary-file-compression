//
// Created by yitzk on 9/20/2026.
//

#include<../include/BitWriter.h>
#include<gtest/gtest.h>

#define BUFFER_SIZE 1<<22 // 4MB

class TestBitWriter : public ::testing::Test
{
protected:
    std::unique_ptr<std::array<uint8_t, BUFFER_SIZE>> buffer =
        std::make_unique<std::array<uint8_t, BUFFER_SIZE>>();

    uint8_t* buffer_ptr;

    BitWriter bit_writer;

    TestBitWriter() : bit_writer(buffer->data()), buffer_ptr(buffer->data())
    {
    }

    [[nodiscard]] uint8_t get_offset() const
    {
        return bit_writer.offset;
    }
    [[nodiscard]] uint8_t* get_iter() const
    {
        return bit_writer.iter;
    }
};

TEST_F(TestBitWriter, AssertOnTooManyBits) {
    std::cout << "[INFO] Death Test: Prevent writing > 32 bits at once.\n";
    // Verifies that the BitWriter correctly asserts and crashes if instructed
    // to write more than 32 bits, which is outside its supported bounds.
    EXPECT_DEATH(
        bit_writer.write_bits_to_buffer(0x1234, 33),
        "The bit writer supports writing at most 32 bits at a time");
    std::cout << "*************** Finish test ***********\n\n";
}


TEST_F(TestBitWriter, Test_write_byte_array_Basic)
{
    std::cout << "[INFO] Basic Test: Write entire byte array directly.\n";
    // Verifies that the writer can dump a full array of bytes into the buffer
    // correctly, overriding any previous data (simulated with i+5).
    int array_size = 100;
    uint8_t byte_array[array_size];

    for (int i = 0; i < array_size; i++)
    {
        byte_array[i] = i;
        (*buffer)[i] = i + 5; // Fill buffer with garbage to test override
    }
    bit_writer.write_byte_array(byte_array, array_size);

    for (int i = 0; i < array_size; i++)
    {
        EXPECT_EQ((*buffer)[i], byte_array[i]);
    }
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_byte_array_EdgeCase1)
{
    std::cout << "[INFO] Edge Case: Write empty byte array (size 0).\n";
    // Ensures that attempting to write 0 bytes does not alter the buffer
    // or advance the internal pointers incorrectly.
    int array_size = 100;
    uint8_t byte_array[array_size];
    for (int i = 0; i < array_size; i++)
    {
        byte_array[i] = i;
        (*buffer)[i] = i + 5;
    }

    bit_writer.write_byte_array(byte_array, 0);

    for (int i = 0; i < array_size; i++)
    {
        EXPECT_NE((*buffer)[i], byte_array[i]);
    }
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_byte_array_EdgeCase2)
{
    std::cout << "[INFO] Edge Case: Write a single byte array.\n";
    // Tests the minimal possible valid state of the array writer (1 byte).
    int array_size = 1;
    uint8_t byte_array[array_size];
    for (int i = 0; i < array_size; i++)
    {
        byte_array[i] = i;
        (*buffer)[i] = i + 5;
    }

    bit_writer.write_byte_array(byte_array, array_size);

    for (int i = 0; i < array_size; i++)
    {
        EXPECT_EQ((*buffer)[i], byte_array[i]);
    }
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_bits_to_buffer_Basic)
{
    std::cout << "[INFO] Basic Test: Write exactly 8 bits (1 full byte).\n";
    // Verifies that writing exactly 8 bits populates a single byte in the buffer
    // correctly without shifting issues.
    uint32_t bit_var = 0b11111111;
    uint8_t num_bits = 8;

    bit_writer.write_bits_to_buffer(bit_var, num_bits);

    EXPECT_EQ((*buffer)[0], bit_var);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_bits_to_buffer_Advance1)
{
    std::cout << "[INFO] Advance Test: Write 0 bits.\n";
    // Checks that the BitWriter gracefully handles a request to write 0 bits
    // without altering the current buffer or internal state.
    uint32_t bit_var = 0b11111111;
    uint8_t num_bits = 0;

    (*buffer)[0] = 0;

    bit_writer.write_bits_to_buffer(bit_var, num_bits);

    EXPECT_EQ((*buffer)[0], 0);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_bits_to_buffer_Advance2)
{
    std::cout << "[INFO] Advance Test: Write 13 bits (crossing a byte boundary).\n";
    // Validates that the writer correctly spills bits over from the first byte
    // into the second byte when the payload exceeds 8 bits.
    uint32_t bit_var = 0b1010101010101;
    uint8_t num_bits = 13;

    (*buffer)[0] = 0;
    (*buffer)[1] = 0;

    bit_writer.write_bits_to_buffer(bit_var, num_bits);
    uint16_t first_two_bytes_buffer;
    memcpy(&first_two_bytes_buffer, (*buffer).data(), 2);
    EXPECT_EQ(first_two_bytes_buffer, bit_var);
    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_write_bits_to_buffer_Advance3)
{
    std::cout << "[INFO] Advance Test: Multiple chained writes and offset tracking.\n";
    // Tests a complex scenario: writing a sequence of 5-bit chunks multiple times,
    // causing the internal offset to wrap around bytes repeatedly.
    // Expectation: Both the data layout and the internal pointers/offsets must be flawless.

    uint32_t bit_var1 = 0;
    for (int i = 0; i < 32; i += 2)
    {
        bit_var1 += 1 << i;
    }

    uint32_t bit_var2 = 0b1111111101;

    for (int i = 0; i < 5; i++)
    {
        (*buffer)[i] = 0;
    }

    int num_writes = 6;
    for (int i = 0; i < num_writes; i++)
    {
        bit_writer.write_bits_to_buffer(bit_var1, 5);
        bit_var1 = bit_var1 >> 5;

        EXPECT_EQ(get_offset(), ((i + 1) * 5) % 8);
        EXPECT_EQ(get_iter(), buffer_ptr + (((i + 1) * 5) / 8));
    }

    bit_writer.write_bits_to_buffer(bit_var2, 9);

    EXPECT_EQ(get_offset(), 7);
    EXPECT_EQ(get_iter(), buffer_ptr + 4);

    uint32_t buffer_first_4_byte_data = *((uint32_t*)buffer_ptr);
    uint8_t buffer_next_byte = *(buffer_ptr + 4);

    bit_var1 = 0;
    for (int i = 0; i < 32; i += 2)
    {
        bit_var1 += 1 << i;
    }

    EXPECT_EQ(buffer_first_4_byte_data, bit_var1);
    EXPECT_EQ(buffer_next_byte, bit_var2 >> 3);

    std::cout << "*************** Finish test ***********\n\n";
}

// -------------------------------------------------------------------------
// NEW CRITICAL TESTS FOR DEFLATE STABILITY
// -------------------------------------------------------------------------

TEST_F(TestBitWriter, Test_write_bits_to_buffer_MaxBitsCrossing)
{
    std::cout << "[INFO] Critical Edge Case: Writing exactly 32 bits across 5 bytes.\n";
    // Tests what happens when we write the maximum allowed payload (32 bits)
    // when the buffer is already partially filled (offset > 0).
    // Expectation: The writer must cleanly spread the 32 bits across 5 distinct bytes
    // without memory corruption or truncating the top bits.

    // Move internal offset to 4 bits
    bit_writer.write_bits_to_buffer(0b1111, 4);

    // Now write a full 32-bit pattern
    uint32_t payload = 0xAABBCCDD;
    bit_writer.write_bits_to_buffer(payload, 32);

    // Reconstruct the 32 bits from the buffer (shifting back the 4 offset bits)
    uint64_t written_data;
    memcpy(&written_data, buffer_ptr, 5); // Read 5 bytes to cover the spillover
    uint32_t extracted_payload = (written_data >> 4) & 0xFFFFFFFF;

    EXPECT_EQ(extracted_payload, payload);
    EXPECT_EQ(get_offset(), 4); // Offset should return to 4 after writing exactly 32 bits
    EXPECT_EQ(get_iter(), buffer_ptr + 4); // Iterator advanced by exactly 4 bytes

    std::cout << "*************** Finish test ***********\n\n";
}

TEST_F(TestBitWriter, Test_DirtyBufferMemoryHandling)
{
    std::cout << "[INFO] Memory Safety Test: Writing into dirty buffer memory.\n";
    // Verifies that when writing into a buffer array that already contains garbage data
    // (e.g., from a previous file compression cycle), the writer properly overwrites
    // the memory with 0s for untouched bits instead of OR-ing them with garbage.

    // Dirty the buffer with all 1s (0xFF)
    for(int i = 0; i < 4; i++) {
        (*buffer)[i] = 0xFF;
    }

    // Write a clean 16-bit payload
    uint32_t payload = 0xAAAA;
    bit_writer.write_bits_to_buffer(payload, 16);

    uint16_t result;
    memcpy(&result, buffer_ptr, 2);

    // Expectation: the dirty 1s must be gone, leaving exactly our payload
    EXPECT_EQ(result, payload);

    std::cout << "*************** Finish test ***********\n\n";
}