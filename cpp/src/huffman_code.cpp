//
// Created by yitzk on 8/19/2026.
//

#include "../include/huffman_code.h"

#include <iostream>


// TODO implemetnt securaty mesures for example prevent ZIP bomb

void Huffman_code::compress(const std::string& file_path,
                            const std::vector<::CodedVec>& coded_vecs,
                            std::uint64_t original_file_size)
{
    // init the file_writer and bit writer
    binary_io::FileWriter file_writer(file_path);

    BitWriter bit_writer(file_writer.get_buffer());

    // write global header
    bit_writer.write_byte_array((uint8_t*)(&original_file_size),8);
    uint64_t num_vecs = coded_vecs.size();
    bit_writer.write_byte_array((uint8_t*)(&num_vecs),8);

    // compress the data
    for (int i = 0; i < coded_vecs.size(); i++)
    {
        compress_block(coded_vecs[i], bit_writer);

        // flush the block onto the file and clear the buffer
        file_writer.flush_buffer_to_file(bit_writer.num_bytes_writen_to_buffer());
        bit_writer.reset();
    }
}

std::vector<CodedVec> Huffman_code::decompress(const std::string& file_path)
{
    std::vector<CodedVec> coded_vecs;

    // the file that the compressed data is in.
    binary_io::FileReader file_reader(file_path);
    finished_file = false;

    // read from the file the first 4MB(or less if the file is smaller
    file_reader.slide_window();

    BitReader bit_reader(file_reader.get_buffer());

    // a pointer to the end of the buffer ment to prevent overflowed reading or reading unread bytes
    // it indicates when we need to read more data from the file into the buffer

    bit_reader.set_safe_end(BUFFER_SIZE - 8);

    // first 8 bytes hold the original file size
    uint64_t original_file_size = *((uint64_t*)bit_reader.get_iter()); // TODO return also this
    bit_reader.advance_buffer(sizeof(uint64_t) * 8);

    // second 8 bytes hold the number of blocks coded
    uint64_t num_bloks = *((uint64_t*)bit_reader.get_iter());
    bit_reader.advance_buffer(sizeof(uint64_t) * 8);

    // this var tracks the number of bytes in the original file we uncompressed so far
    while (num_bloks > 0)
    {
        coded_vecs.emplace_back(decompress_block(file_reader, bit_reader));
        num_bloks--;
        // if the read stoped in the middle of a byte in the buffer then alighn the reader
        // to start at the begining of the next byte (the blocks always start at the begining of a byte)
        bit_reader.alighn_reader_to_byte();
    }

    return coded_vecs;
}

void Huffman_code::decode_window_length(::CodedVec& coded_vec, uint32_t symbol, BitReader& bit_reader)
{
    uint32_t extra_val = bit_reader.peak_bits(symbolToLen_num_extra_bits[symbol]);
    bit_reader.advance_buffer(symbolToLen_num_extra_bits[symbol]);
    uint32_t val = extra_val + symbolToLenRange_table[symbol];

    coded_vec.push_back(val + WINDOW_SYMBOL_OFFSET_IN_TABLE);
}

void Huffman_code::decode_distance(::CodedVec& coded_vec, uint32_t symbol, BitReader& bit_reader)
{
    uint32_t extra_val = bit_reader.peak_bits(symbolToDist_num_extra_bits[symbol]);
    bit_reader.advance_buffer(symbolToDist_num_extra_bits[symbol]);
    uint32_t val = extra_val + symbolToDistanceRange_table[symbol];

    coded_vec.push_back(val);
}

CodedVec Huffman_code::decompress_block(binary_io::FileReader& file_reader, BitReader& bit_reader)
{
    CodedVec coded_vec;

    // create maps that map from 15 bits to the symbol that it's huffman code start with those 15 bits
    std::vector<Decode> map_15bit_lit_len = create_15bit_to_symbol_table(LITERAL_AND_LEN_NUM_SYMBOLS, bit_reader);
    std::vector<Decode> map_15bit_dist = create_15bit_to_symbol_table(DISTANCE_NUM_SYMBOLS, bit_reader);

    while (true)
    {
        // if we have reached the end of the buffer we need to load more bytes from the file into it
        if (!bit_reader.safe_read())
        {
            read_new_data_into_buffer(file_reader, bit_reader);
        }

        // get the next 15 bits in the buffer
        uint16_t next_chunk = bit_reader.peak_bits(15);
        uint32_t symbol = map_15bit_lit_len[next_chunk].symbol;

        bit_reader.advance_buffer(map_15bit_lit_len[next_chunk].num_bits);

        // we reached the end of the vector
        if (symbol == EOF_SYMBOL)
        {
            coded_vec.push_back(EOF_SYMBOL);
            break;
        }

        if (symbol < 256) coded_vec.push_back(symbol);

        else
        {
            // we decode window lenght
            decode_window_length(coded_vec, symbol, bit_reader);

            // next we encode distance
            next_chunk = bit_reader.peak_bits(15);
            symbol = map_15bit_dist[next_chunk].symbol;

            bit_reader.advance_buffer(map_15bit_dist[next_chunk].num_bits);


            decode_distance(coded_vec, symbol, bit_reader);
        }
    }
    return coded_vec;
}

std::vector<uint16_t> Huffman_code::read_code_len_table(size_t num_symbols, BitReader& bit_reader)
{
    // the vector to put the lengths in
    std::vector<uint16_t> code_len_table;

    // iterate to fill the vector with the code lengths
    for (size_t i = 0; i < num_symbols; i++)
    {
        uint64_t len = bit_reader.peak_bits(8);

        bit_reader.advance_buffer(8);

        code_len_table.push_back(len);
    }
    return code_len_table;
}

std::vector<Decode> Huffman_code::create_15bit_to_symbol_table(const int num_symbols, BitReader& bit_reader)
{
    // get the code lengths of each symbol and put it in a table
    std::vector<uint16_t> code_length_table = read_code_len_table(num_symbols, bit_reader);

    // get the canonial code for each tree
    // map from symbol(index of the vector) to (len,binary_code)
    std::vector<HuffmanCode> canonial_code = HuffmanBuilder::get_canonial_huffman_code(code_length_table, MAX_CODE_LEN);


    std::vector<Decode> lookup_table(1 << MAX_CODE_LEN);

    // for each symbol fill in all the indexes that start with the value of the huffman code with the symbol and length
    // of that symbol's huffman code
    for (int i = 0; i < canonial_code.size(); i++)
    {
        // if this symbol does not apear in the coded file then we dont need to calclate anything for it
        if (canonial_code[i].len_code == 0) continue;

        // the offset between each index of the table with the binary prefix of huffman_code[i].huffman_code
        // for example suppose the huffman code 011 so the next value that strats with 011 is 1011 which is 1000 + 011.
        uint16_t offset = 1 << canonial_code[i].len_code;

        // the number of values in the encription_table with the prefix huffman_code[i].huffman_code
        uint16_t num_iterations = 1 << (MAX_CODE_LEN - canonial_code[i].len_code);

        // the first index in the table we start at is the binary value of huffman_code[i].huffman_code
        // for example for the code 11000 then we will start at 000000000011000 index in encription_table
        uint16_t index_in_table = canonial_code[i].huffman_code;

        // filling encription_table indexes that start with prefix  huffman_code[i].huffman_code
        for (int j = 0; j < num_iterations; j++, index_in_table += offset)
        {
            lookup_table[index_in_table].symbol = i;
            lookup_table[index_in_table].num_bits = canonial_code[i].len_code;
        }
    }

    return lookup_table;
}

void Huffman_code::read_new_data_into_buffer(binary_io::FileReader& file_reader, BitReader& bit_reader)
{
    // copy the last 8 bytes in the buffer to the start and update bit_reader acording to the place it was.
    bit_reader.cycle_buffer(8);

    // fill in the buffer to the max from the first 8 bytes and update the fields

    if(finished_file) throw std::runtime_error("CRITICAL: Reached unexpected end of file! Bitstream lost sync.");

    finished_file = !(file_reader.read_bytes(file_reader.get_buffer() + 8, BUFFER_SIZE - 8));
}

void Huffman_code::compress_block(const CodedVec& coded_vec, BitWriter& bit_writer)
{
    // seperate the vectors
    CodedVec literal_and_len_vec;
    CodedVec distance_vec;
    split_vec(literal_and_len_vec, distance_vec, coded_vec);

    // create the code
    std::vector<HuffmanCode> literalLen_code = HuffmanBuilder::get_canonial_huffman_code(
        literal_and_len_vec, literal_and_window_mapper, LITERAL_AND_LEN_NUM_SYMBOLS, MAX_CODE_LEN);

    std::vector<HuffmanCode> distance_code = HuffmanBuilder::get_canonial_huffman_code(
        distance_vec, distance_mapper, DISTANCE_NUM_SYMBOLS, MAX_CODE_LEN);


    // fill the table WindowLengthToCode
    create_windowLenToCode_table(literalLen_code);


    // write into the buffer the code lengths of the huffman code
    bit_writer.write_byte_array(HuffmanBuilder::get_code_len_table(literalLen_code).data(),
                                LITERAL_AND_LEN_NUM_SYMBOLS);
    bit_writer.write_byte_array(HuffmanBuilder::get_code_len_table(distance_code).data(),
                                DISTANCE_NUM_SYMBOLS);


    // code the coded_vec_ into the buffer and from there flushed to the file
    code_vec(coded_vec, literalLen_code, distance_code, bit_writer);
}

void Huffman_code::code_vec(const CodedVec& coded_vec,
                            std::vector<HuffmanCode>& literalLen_code,
                            std::vector<HuffmanCode>& distance_code, BitWriter& bit_writer)
{
    // code the vector
    for (uint32_t i = 0; i < coded_vec.size(); i++)
    {
        // the next value in the vector
        uint32_t val = coded_vec[i];

        // if val is a literal
        if (val <= EOF_SYMBOL)
        {
            // getting the len and code of the huffman code of 'val'
            uint16_t code_len = literalLen_code[val].len_code;
            uint64_t huffman_code = literalLen_code[val].huffman_code;
            bit_writer.write_bits_to_buffer(huffman_code, code_len);
        }

        // else val is a window len
        else
        {
            // write the code for window len into the buffer
            uint32_t window_len = val - WINDOW_SYMBOL_OFFSET_IN_TABLE;
            bit_writer.write_bits_to_buffer(windowLenToCode[window_len].huffman_code,
                                            windowLenToCode[window_len].huffman_len);
            bit_writer.write_bits_to_buffer(windowLenToCode[window_len].extra_bits_val,
                                            windowLenToCode[window_len].extra_bits_len);

            // get the distance
            val = coded_vec[++i];

            // get the distance info (the symbol in the tree, the extra bits, and the len of the diffarance
            DistanceEncodeInfo dist_info = map_distance_to_symbol(val);
            uint16_t huffman_code = distance_code[dist_info.symbol].huffman_code;
            uint8_t code_len = distance_code[dist_info.symbol].len_code;

            // write the data into the buffer
            bit_writer.write_bits_to_buffer(huffman_code, code_len);
            bit_writer.write_bits_to_buffer(dist_info.extra_bits_val, dist_info.extra_bits_len);
        }
    }
}

uint16_t Huffman_code::map_window_len_to_symbol(uint32_t window_len)
{
    // If window length is in the range of [4, 22], we map it directly.
    // We subtract 4 to align it to a 0-based index, and add the offset
    // because the first 257 symbols are reserved for literals and EOF.
    if (window_len <= 18 + 4)
    {
        return window_len - 4 + WINDOW_SYMBOL_OFFSET_IN_TABLE;
    }

    // Otherwise, we calculate the exponent range to find the corresponding symbol.
    window_len = window_len - 21; // Normalize to the first exponential jump
    uint32_t msb = std::bit_width(window_len) - 1; // Number of range jumps (*2 each time)

    // Return the number of jumps plus the 18 direct symbols before it, plus the base offset
    return msb + 18 + WINDOW_SYMBOL_OFFSET_IN_TABLE;
}

DistanceEncodeInfo Huffman_code::map_distance_to_symbol(uint32_t val)
{
    // Base case: The first 16 distances (1 to 16) map directly to symbols 0 to 15.
    // They don't require any extra bits.
    if (val <= 16)
    {
        return {val - 1, 0, 0};
    }

    // For distances > 16, we use a bitwise trick to find the symbol.
    // Subtracting 13 aligns the values so that their Most Significant Bit (MSB)
    // strictly defines the exponential "tier" (group of ranges) they belong to.
    // (e.g., distances 17-20 fall into the tier where MSB is 2, 21-28 -> MSB is 3, etc.)
    val = val - 13;
    uint32_t msb = std::bit_width(val) - 1;

    // Every tier contains exactly two symbols.
    // We extract the second most significant bit to determine which of the two
    // symbols in the current tier this distance belongs to (0 for the first, 1 for the second).
    uint8_t b = (val >> (msb - 1)) & 1;

    DistanceEncodeInfo info{};

    // Calculate the final symbol:
    // (msb - 2) * 2 calculates the base index of the tier.
    // We add 16 to offset the direct symbols we skipped.
    // Finally, we add 'b' to select the correct symbol inside the pair.
    info.symbol = (((msb - 2) * 2) + 16) + b;
    info.extra_bits_len = msb - 1;

    // The remaining lower bits represent the exact position inside the symbol's range.
    uint32_t mask = (1 << info.extra_bits_len) - 1;
    info.extra_bits_val = val & mask;

    return info;
}

void Huffman_code::create_windowLenToCode_table(const std::vector<HuffmanCode>& canonial_code)
{
    for (int window_len = 4; window_len < windowLenToCode.size(); window_len++)
    {
        int symbol = map_window_len_to_symbol(window_len); //symbol in the literal and window len table
        uint32_t extra_bit_val = (window_len) - symbolToLenRange_table[symbol];

        windowLenToCode[window_len].huffman_code = canonial_code[symbol].huffman_code;
        windowLenToCode[window_len].huffman_len = canonial_code[symbol].len_code;
        windowLenToCode[window_len].extra_bits_val = extra_bit_val;
        windowLenToCode[window_len].extra_bits_len = symbolToLen_num_extra_bits[symbol];
    }
}

void Huffman_code::split_vec(CodedVec& literal_and_len_vec, CodedVec& distance_vec, const CodedVec& coded_vec)
{
    for (size_t i = 0; i < coded_vec.size(); i++)
    {
        literal_and_len_vec.push_back(coded_vec[i]);
        if (coded_vec[i] > EOF_SYMBOL) distance_vec.push_back(coded_vec[++i]);
    }
}

uint32_t Huffman_code::literal_and_window_mapper(uint32_t val)
{
    if (val <= 256) return val;

    return map_window_len_to_symbol(val - WINDOW_SYMBOL_OFFSET_IN_TABLE);
}

uint32_t Huffman_code::distance_mapper(uint32_t val)
{
    // Base case: The first 16 distances (1 to 16) map directly to symbols 0 to 15.
    // They don't require any extra bits.
    if (val <= 16)
    {
        return val - 1;
    }

    // For distances > 16, we use a bitwise trick to find the symbol.
    // Subtracting 13 aligns the values so that their Most Significant Bit (MSB)
    // strictly defines the exponential "tier" (group of ranges) they belong to.
    // (e.g., distances 17-20 fall into the tier where MSB is 2, 21-28 -> MSB is 3, etc.)
    val = val - 13;
    uint32_t msb = std::bit_width(val) - 1;

    // Every tier contains exactly two symbols.
    // We extract the second most significant bit to determine which of the two
    // symbols in the current tier this distance belongs to (0 for the first, 1 for the second).
    uint8_t b = (val >> (msb - 1)) & 1;

    // Calculate the final symbol:
    // (msb - 2) * 2 calculates the base index of the tier.
    // We add 16 to offset the direct symbols we skipped.
    // Finally, we add 'b' to select the correct symbol inside the pair.
    return (((msb - 2) * 2) + 16) + b;
}