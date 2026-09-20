//
// Created by yitzk on 8/19/2026.
//
#ifndef HUFFMAN_CODE_H
#define HUFFMAN_CODE_H

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "binary_io.h"
#include "HuffmanBuilder.h"
#include "BitWriter.h"
#include "BitReader.h"

using CodedVec = std::vector<uint32_t>;

struct WindowLengthToCode
{
    uint16_t huffman_code; // the binary code
    uint8_t huffman_len; // the number of bytes in the code
    uint32_t extra_bits_val;
    // the value of the len - range (each length is in a range look at constructor implementation to see the ranges
    uint8_t extra_bits_len; // the number of bits of extra_bits_val
};

struct DistanceEncodeInfo
{
    uint32_t symbol;
    uint32_t extra_bits_val;
    uint32_t extra_bits_len;
};

struct Decode
{
 Decode(uint8_t symbol, uint8_t num_bits): symbol(symbol), num_bits(num_bits)
 {
 }

 Decode(): symbol(-1), num_bits(-1)
 {
 }

 bool operator <(const Decode& other) const
 {
  return num_bits < other.num_bits;
 }

 uint8_t symbol;
 uint8_t num_bits;
};

/**
 * @class huffman_code
 *
 * @brief This class holds all the logic to encode LZSS vector into a binary file using
 *        Canonical Huffman coding, and to decode the binary file back into LZSS vector.
 *
 * The class implements a Deflate-like compression architecture using data blocks,
 * two separate Huffman trees:
 * One - for literals, windows lenghts.
 * Two - distances, that is the index that the window starts from.
 * The windows lenghts and distances are coded with ranges and not exact numbers
 * via Base + Extra Bits encoding (look bellow at Decoding stage to understand).
 *
 * =========================================================================================
 * 1. THE CODING FORMAT
 * =========================================================================================
 * The file is structured as a Global Header followed by one or more Compressed Blocks.
 *
 * [Global Header]
 * <original_file_size>  -> 8 bytes (uint64_t). The total uncompressed size of the original file.
 * <num_blocks_coded>  -> 8 bytes (uint64_t).
 *
 * [Block i] (Repeats until End Of File)
 * <Tree 1 Definition (Literals & Lengths)>
 *      <bit_lengths_1>  -> Array of 1 byte lengths used to reconstruct the Canonical Huffman tree.
 *                          (Symbol values:
                                             0-255:   Literals,
                                             256:     EOF(end of file)
                                             256-284: Length Ranges from ). // TODO add the discription of the ranges
 *
 * <Tree 2 Definition (Distances)>
 *      <bit_lengths_2>  -> Array of lengths used to reconstruct the Canonical Huffman tree.
 *                          (Symbol values represent Distance Ranges).
 *                          // TODO add the discription of the ranges
 *
 * <Bitstream (The Encoded LZSS Data)>
 *      The bitstream contains a sequence of encoded elements. For each element:
 *      a. 'literal' (< 256):     Encode <Huffman Code of literal from Tree 1>
 *      b. 'length' (length+diff): Encode <Huffman Code of length Symbol from Tree 1> + <Extra Bits for diff>
 *                                Encode <Huffman Code of Distance Symbol from Tree 2> + <Extra Bits for diff>
 *      c. 'end of block':        Encode <Huffman Code of 256 from Tree 1>
 *
 * =========================================================================================
 * 2. THE DECODING STATE MACHINE
 * =========================================================================================
 * To decode the bitstream, the decoder acts as a deterministic state machine:
 *
 * State 1: Read a Huffman code from Tree 1.
 *          - If symbol < 256:  It's a Literal. Write it to the output buffer. Loop back to State 1.
 *          - If symbol == 256: It's the End of Block marker. Stop decoding this block.
 *          - If symbol > 256:  It's a Length Category. Proceed to State 2.
 *
 * State 2: Length Extraction.
 *          - Look up the Base Length and Num Extra Bits for the symbol (using a hardcoded table).
 *          - Read <Num Extra Bits> raw bits from the bitstream.
 *          - Actual Length = Base Length + Value of Extra Bits. Proceed to State 3.
 *
 * State 3: Distance Extraction & Copy.
 *          - Read a Huffman code from Tree 2 (Distance Category).
 *          - Look up the Base Distance and Num Extra Bits for the symbol.
 *          - Read <Num Extra Bits> raw bits from the bitstream.
 *          - Actual Distance = Base Distance + Value of Extra Bits.
 *          - Copy <Actual Length> bytes starting from <Actual Distance> bytes backwards in the output buffer.
 *          - Loop back to State 1.
 *
 * =========================================================================================
 * 3. ARCHITECTURE NOTES
 * =========================================================================================
 * - Block Size: Configured to 4MB. This bounds memory consumption and ensures that the Huffman
 *   trees adapt to local statistics of the file (e.g., code vs. text).
 * - Canonical Huffman: The tree definitions written to the file do NOT contain the actual codes
 *   or tree structure. They only contain the bit-length of each symbol. The exact codes are
 *   reconstructed deterministically on the decoding side.
 * - Raw Bits: Extra bits are written/read natively without Huffman encoding because the distribution
 *   within a specific range is uniform (pure entropy).
 */
class Huffman_code
{
public:
    /**
     * Constructor, it initalizes all the used data structures needed throughout its life.
     */
    explicit Huffman_code();

    /**
     *
     * This function gets all the data from a lempel zivSS compression, and uses huffman code to compress it into binary
     * into a file named 'file_path'.
     *
     * @param file_path the file path to dump the compressed file into
     * @param coded_vecs the vector that hold the lempel ziv compression (each vector corespondes to a block of at most 4MB of the file)
     * @param num_bytes_in_block_before_compression holds to each vector in coded_vecs the number of bytes in the original file it compresses.
     * @param original_file_size the number of bytes of the whole file
     */
    void compress(const std::string& file_path,
                  std::vector<CodedVec>& coded_vecs,
                  std::vector<uint32_t>& num_bytes_in_block_before_compression,
                  std::uint64_t original_file_size);

    /**
     * This function decompresses 'file_path' file, into lempel ziv code.
     * this method assumes that the format of the file is correct.
     *
     * @param file_path the file that the compressed file is at
     * @return a vector of coded vecs each coded vec is a block of compressed data.
     */
    std::vector<CodedVec> decompress(const std::string& file_path);

    /**
     * Rests the data structures that this object holds for a new compression.
     * It must be called before each compression call IF the object was used before to compress or decompress somthing else.
     */
    void clear();

private:
    //#################### FUNCTION TO WRITE INTO THE FILE #####################
    /**
     * this function codes a vector that was coded using lempel ziv into binary code using huffman code
     * @param coded_vec the coded vec to code into binary.
     * @param literalLen_code the huffman code to use to literals and window lengths
     * @param distance_code the huffman code to use for distances
     *
     * @return the number of bytes needed to compress this vec
     */
    static void code_vec(const ::CodedVec& coded_vec,
                               std::vector<HuffmanCode>& literalLen_code,
                               std::vector<HuffmanCode>& distance_code,
                               BitWriter& bit_writer);
    //#################### FUNCTION TO WRITE INTO THE FILE #####################


    //############# HUFFMAN BINARY CODE HELPER FUNCTIONS########################
    ::CodedVec decompress_block(binary_io::FileReader& file_reader, BitReader& bit_reader);



    /**
     * this function codes into binary the the coded vec into a block and writes it into the file.
     * @param coded_vec the coded vec of this block
     * @param num_bytes_compressed_in_block the number of bytes of the original uncompressed file this block compresses
     */
    static void compress_block(CodedVec& coded_vec, uint32_t num_bytes_compressed_in_block, BitWriter& bit_writer);

    static uint32_t literal_and_window_mapper(uint32_t val);

    static uint32_t distance_mapper(uint32_t val);
    //############# HUFFMAN BINARY CODE HELPER FUNCTIONS########################


    //############## DATA STRUCTURES METHODS ################
    static uint16_t map_window_len_to_symbol(uint32_t window_len);

    static DistanceEncodeInfo map_distance_to_symbol(uint32_t val);

    /**
     * This function gets the huffman code and is responsible to create a map that will map every number with the prefix
     * of that huffman code to the symbol that that hufmman code encodes and the number of bits of that huffman code.
     *
     * @return the Decoding vector
     */
    static std::vector<Decode> create_15bit_to_symbol_table(int num_symbols, BitReader& bit_reader);

    /**
     * This function extracts the length table from the block's code.
     * 
     * @param num_symbols the number of symbols to extract their length
     * @return a vector that maps each symbol(the index) to its prefix free code length
     */
    static std::vector<uint16_t> read_code_len_table(size_t num_symbols, BitReader& bit_reader);

    /**
     *  This function gets the current symbol read that encodes a window length, and reads from the buffer the
     *  the rest of the data to decode the actual coded window length and writes it into the vector
     *  
     * @param block_code the data structure that holds the decoded vector
     * @param symbol the symbol of the window length we want to decode
     */
    static void decode_window_length(::CodedVec& block_code, uint8_t symbol, BitReader& bit_reader);

    /**
     * This function gets the current symbol read that encodes a distance and reads from the buffer  the
     * the rest of the data to decode the actual coded distance and writes it into the vector
     * @param block_code the data structure that holds the decoded vector
     * @param symbol the symbol of the distance we want to decode
     */
    static void decode_distance(::CodedVec& block_code, uint8_t symbol, BitReader& bit_reader);

    /**
     * this function reads new data into the buffer from the file, making sure that the data that was not read yeat is
     * saved in the new buffer.
     *
     * @param file_reader the file to read the new data from
     * @param bit_reader the handle to read single bits out of the file
     */
    static void read_new_data_into_buffer(binary_io::FileReader& file_reader, BitReader& bit_reader);

    /**
     * This function fills in the field 'windowLengthToCode' which is a table that lets us access in O(1) all the
     * relevent data to encode a window lenght without needing to calculate anything.
     * The reason we have this method for window lengths and not for distances is that there are 2069 different window
     * lenghts but there can be up to 4MB of distances (oproxemetly 4 milion bytes) so having a table for that is
     * inpracticle.
     * @param canonial_code the prefix free code for each symbol
     */
    static void create_windowLenToCode_table(const std::vector<HuffmanCode>& canonial_code);

 static void split_vec(CodedVec& literal_and_len_vec, CodedVec& distance_vec, const CodedVec& coded_vec);
    //############## DATA STRUCTURES METHODS ################


    //#################### FIELDS ###################
    static const int MAX_CODE_LEN = 15;
    const static uint8_t EOF_SYMBOL;
    static const uint16_t WINDOW_OFFSET = 257;

    // number of symbols in each huffman tree
    static const int LITERAL_AND_LEN_NUM_SYMBOLS = 285;
    static const int DISTANCE_NUM_SYMBOLS = 56;

    // maps from symbol of the huffman code to the range of number it represents
    static std::array<uint32_t, LITERAL_AND_LEN_NUM_SYMBOLS> symbolToLenRange_table;
    static std::array<uint32_t, DISTANCE_NUM_SYMBOLS> symbolToDistanceRange_table;

    static std::array<WindowLengthToCode, 2069> windowLenToCode;

    static std::array<uint8_t, LITERAL_AND_LEN_NUM_SYMBOLS - 256> symbol_to_len_num_extra_bits;
    static std::array<uint8_t, DISTANCE_NUM_SYMBOLS> symbol_to_dist_num_extra_bits;
    //#################### FIELDS ###################
};
#endif //HUFFMAN_CODE_H