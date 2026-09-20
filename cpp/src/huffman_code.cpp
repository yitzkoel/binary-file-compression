//
// Created by yitzk on 8/19/2026.
//

#include "../include/huffman_code.h"


// TODO implemetnt securaty mesures for example prevent ZIP bomb

Huffman_code::Huffman_code()
{
    // setup symbolToLenRange_table
    uint32_t jump = 1;
    uint8_t extra_bit_len = 0;
    for (int i = 0; i < LITERAL_AND_LEN_NUM_SYMBOLS; i++)
    {
        // if we are at the literal and EOF symbols add the same value to the table
        if (i <= 255)
        {
            symbolToLenRange_table[i] = i;
            continue;
        }
        // if we are at the symbols that represend window lenght are from index 257 - 285 then:


        int window_len_symbol = i - 256;
        // if we are the first 18 window len symbols then we just keep the length of that symbol from 4 to 22
        if (window_len_symbol <= 18)
        {
            symbolToLenRange_table[i] = 4 + window_len_symbol;
            symbol_to_len_num_extra_bits[window_len_symbol] = extra_bit_len;
        }
        // else we are at the ranges: 2, 4, 8, 16, 32, 64, 128, 265, 512, 1024 (plus the base 22) 
        else
        {
            symbolToLenRange_table[i] = symbolToLenRange_table[i - 1] + jump;
            jump = jump << 1; // mult by 2

            symbol_to_len_num_extra_bits[window_len_symbol] = ++extra_bit_len;
        }
    }


    // setup symbolToDistanceRange_table
    jump = 1;
    extra_bit_len = 0;
    for (int i = 0; i < DISTANCE_NUM_SYMBOLS; i++)
    {
        if (i < 16)
        {
            symbolToDistanceRange_table[i] = i + 1;
            symbol_to_dist_num_extra_bits[i] = extra_bit_len;
        }
        else
        {
            symbolToDistanceRange_table[i] = symbolToDistanceRange_table[i - 1] + jump;
            if (i % 2 == 0)
            {
                jump = jump << 1;
                extra_bit_len++;
            }
            symbol_to_dist_num_extra_bits[i] = extra_bit_len;
        }
    }

    // TODO fill in the fields of the tables num_extra_bytes
}

void Huffman_code::compress(const std::string& file_path,
                            std::vector<::CodedVec>& coded_vecs,
                            std::vector<uint32_t>& num_bytes_in_block_before_compression,
                            std::uint64_t original_file_size)
{
    // init the file_writer and bit writer
    binary_io::FileWriter file_writer(file_path);

    BitWriter bit_writer(file_writer.get_buffer());

    // write global header
    bit_writer.write_bits_to_buffer(original_file_size, 8 * 8);
    bit_writer.write_bits_to_buffer(coded_vecs.size(), 8 * 8);

    // compress the data
    for (int i = 0; i < coded_vecs.size(); i++)
    {
        compress_block(coded_vecs[i], num_bytes_in_block_before_compression[i], bit_writer);

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

    // read from the file the first 4MB(or less if the file is smaller
    file_reader.slide_window();

    BitReader bit_reader(file_reader.get_buffer());

    // a pointer to the end of the buffer ment to prevent overflowed reading or reading unread bytes
    // it indicates when we need to read more data from the file into the buffer
    bit_reader.set_safe_end(file_reader.get_num_bytes_read() - 8);

    // first 8 bytes hold the original file size
    uint64_t original_file_size = bit_reader.read_bits_from_buffer(sizeof(uint64_t) * 8);
    bit_reader.advance_buffer(sizeof(uint64_t) * 8);

    // second 8 bytes hold the number of blocks coded
    uint64_t num_bloks = bit_reader.read_bits_from_buffer(sizeof(uint64_t) * 8);
    bit_reader.advance_buffer(sizeof(uint64_t) * 8);

    // this var tracks the number of bytes in the original file we uncompressed so far
    while (num_bloks > 0)
    {
        coded_vecs.emplace_back(decompress_block(file_reader, bit_reader));
        num_bloks--;
    }

    return coded_vecs;
}

void Huffman_code::decode_window_length(::CodedVec& coded_vec, uint8_t symbol, BitReader& bit_reader)
{
    uint32_t extra_val = bit_reader.read_bits_from_buffer(symbol_to_len_num_extra_bits[symbol]);
    bit_reader.advance_buffer(symbol_to_len_num_extra_bits[symbol]);
    uint32_t val = extra_val + symbolToLenRange_table[symbol];

    coded_vec.push_back(val);
}

void Huffman_code::decode_distance(::CodedVec& coded_vec, uint8_t symbol, BitReader& bit_reader)
{
    uint32_t extra_val = bit_reader.read_bits_from_buffer(symbol_to_dist_num_extra_bits[symbol]);
    bit_reader.advance_buffer(symbol_to_dist_num_extra_bits[symbol]);
    uint32_t val = extra_val + symbolToDistanceRange_table[symbol];

    coded_vec.push_back(val);
}

::CodedVec Huffman_code::decompress_block(binary_io::FileReader& file_reader, BitReader& bit_reader)
{
    ::CodedVec coded_vec;

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
        uint16_t next_chunk = bit_reader.read_bits_from_buffer(15);
        bit_reader.advance_buffer(map_15bit_lit_len[next_chunk].num_bits);
        uint8_t symbol = map_15bit_lit_len[next_chunk].symbol;

        if(symbol == EOF_SYMBOL) break; // we reached the end of the vector

        if (symbol < 256) coded_vec.push_back(symbol);

        else
        {
            // we decode window lenght
            decode_window_length(coded_vec, symbol, bit_reader);

            // next we encode distance
            next_chunk = bit_reader.read_bits_from_buffer(15);
            bit_reader.advance_buffer(map_15bit_dist[next_chunk].num_bits);
            symbol = map_15bit_dist[next_chunk].symbol;

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
        uint64_t len = bit_reader.read_bits_from_buffer(8);

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


    std::vector<Decode> encription_table(2 << MAX_CODE_LEN);

    // for each symbol fill in all the indexes that start with the value of the huffman code with the symbol and length
    // of that symbol's huffman code
    for (int i = 0; i < canonial_code.size(); i++)
    {
        // the offset between each index of the table with the binary prefix of huffman_code[i].huffman_code
        // for example suppose the huffman code 011 so the next value that strats with 011 is 1011 which is 1000 + 011.
        uint16_t offset = 1 << canonial_code[i].len_code;

        // the number of values in the encription_table with the prefix huffman_code[i].huffman_code
        uint16_t num_iterations = 2 << (MAX_CODE_LEN - canonial_code[i].len_code);

        // the first index in the table we start at is the binary value of huffman_code[i].huffman_code
        // for example for the code 11000 then we will start at 000000000011000 index in encription_table
        uint16_t index_in_table = canonial_code[i].huffman_code;

        // filling encription_table indexes that start with prefix  huffman_code[i].huffman_code
        for (int j = 0; j < num_iterations; j++, index_in_table += offset)
        {
            encription_table[index_in_table].symbol = i;
            encription_table[index_in_table].num_bits = canonial_code[i].len_code;
        }
    }

    return encription_table;
}

void Huffman_code::read_new_data_into_buffer(binary_io::FileReader& file_reader, BitReader& bit_reader)
{
    bit_reader.cycle_buffer(8);

    file_reader.get_buffer();

    // fill in the buffer to the max from the first 8 bytes and update the fields
    file_reader.read_bytes(file_reader.get_buffer() + 8, BUFFER_SIZE - 8);
    bit_reader.set_safe_end(file_reader.get_num_bytes_read() - 8);
}


void Huffman_code::compress_block(::CodedVec& coded_vec, uint32_t num_bytes_in_block_before_compression,
                                  BitWriter& bit_writer)
{
    // Add EOF symbol
    coded_vec.push_back(EOF_SYMBOL);

    // seperate the vectors
    ::CodedVec literal_and_len_vec;
    ::CodedVec distance_vec;
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


void Huffman_code::code_vec(const ::CodedVec& coded_vec,
                                  std::vector<HuffmanCode>& literalLen_code,
                                  std::vector<HuffmanCode>& distance_code, BitWriter& bit_writer)
{
    // code the vector
    for (uint32_t i = 0; i < coded_vec.size(); i++)
    {
        // the next value in the vector
        uint32_t val = coded_vec[i];

        // if val is a literal
        if (val <= 255)
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
            uint32_t window_len = val - WINDOW_OFFSET;
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
    // if window lenght is is in the range of [4,22] we just return the symbol is the len minus 4
    // (because we want it to start at 0) plus the window offset becuase we want the first 257 symbols to be literal and EOF.
    if (window_len <= 18 + 4) return window_len - 4 + WINDOW_OFFSET;

    // else we need to calculate the range we fell in to find the symbol
    window_len = window_len - 22; // normalizing to the first range jump
    uint32_t msb = std::bit_width(window_len) - 1; // the number of jumps made(range jumps of *2 each time)
    return msb + 18 + WINDOW_OFFSET; // the number of jumps plus the number of symbols before it pluss the offset
}

DistanceEncodeInfo Huffman_code::map_distance_to_symbol(uint32_t val)
{
    // base case no range in table
    if (val <= 16)
    {
        return {val - 1, 0, 0};
    }
    val = val - 13;
    uint32_t msb = std::__bit_width(val) - 1;

    uint8_t b = (val >> (msb - 1)) & 1;

    DistanceEncodeInfo info{};
    info.symbol = (((msb - 2) * 2) + 16) + b;
    info.extra_bits_len = msb - 1;

    uint32_t mask = (1 << info.extra_bits_len) - 1;
    info.extra_bits_val = val & mask;

    return info;
}


void Huffman_code::create_windowLenToCode_table(
    const std::vector<HuffmanCode>& canonial_code)
{
    for (int window_len = 4; window_len < windowLenToCode.size() + 4; window_len++)
    {
        int symbol = map_window_len_to_symbol(window_len);
        uint32_t extra_bit_val = (window_len) - symbolToLenRange_table[symbol];

        windowLenToCode[window_len].huffman_code = canonial_code[symbol].huffman_code;
        windowLenToCode[window_len].huffman_len = canonial_code[symbol].len_code;
        windowLenToCode[window_len].extra_bits_val = extra_bit_val;
        windowLenToCode[window_len].extra_bits_len = std::__bit_width(extra_bit_val);
    }
}

void Huffman_code::split_vec(CodedVec& literal_and_len_vec, CodedVec& distance_vec, const CodedVec& coded_vec)
{
    for(size_t i = 0; i < coded_vec.size(); i++)
    {
        literal_and_len_vec.push_back(coded_vec[i]);
        if(coded_vec[i] > 255) distance_vec.push_back(coded_vec[++i]);
    }
}

uint32_t Huffman_code::literal_and_window_mapper(uint32_t val)
{
    if (val <= 256) return 256;


    return map_window_len_to_symbol(val);
}

uint32_t Huffman_code::distance_mapper(uint32_t val)
{
    // base case no range in table
    if (val <= 16)
    {
        return val - 1;
    }
    val = val - 13;
    uint32_t msb = std::__bit_width(val) - 1;

    uint8_t b = (val >> (msb - 1)) & 1;

    return (((msb - 2) * 2) + 16) + b;
}
