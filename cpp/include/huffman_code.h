//
// Created by yitzk on 8/19/2026.
//

#ifndef HUFFMAN_CODE_H
#define HUFFMAN_CODE_H

#include <memory>
#include <string>
#include <vector>
#include <cstring>
#include <queue>
#include <bit>
#include <utility>
#include <algorithm>
#include "binary_io.h"

using CodedVec = std::vector<uint32_t>;

struct HuffmanTreeNode
{
    HuffmanTreeNode(uint32_t frequency, uint16_t left, uint16_t right): frequency(frequency), left(left), right(right)
    {
    }

    HuffmanTreeNode(): frequency(0), left(-1), right(-1)
    {
    }

    // note that it is unsighed and -1 is the largest value (111111111111 in binary)
    uint32_t frequency;
    uint16_t left;
    uint16_t right;
};

using huffmanTree = std::vector<HuffmanTreeNode>;

struct HuffmanBuilder
{
    uint8_t len_code;
    uint16_t huffman_code;
};

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

struct LempelZivBlockCode
{
    CodedVec coded_vec_;
    uint32_t num_bytes_compressed_in_block;
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
 *
 * [Block i] (Repeats until End Of File)
 * <uncompressed_size>   -> 3 bytes. The uncompressed size of this specific block (e.g.,4MB is the top limit).
 * <compressed_size>     -> 3 bytes. The number of bytes used to compress the entire block (not includes this 6 bytes of sizes).
 *
 * <Tree 1 Definition (Literals & Lengths)>
 *      <bit_lengths_1>  -> Array of 1 byte lengths used to reconstruct the Canonical Huffman tree.
 *                          (Symbol values:
                                             0-255:   Literals,
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
     * writes into the first 8 bytes of the coded file the uncompressed file size.
     * @param original_file_size the uncompressed file
     */
    void write_global_header(std::uint64_t original_file_size);

    /**
     * This function writes into the buffer the header to a block.
     * @param num_bytes_compressed_in_block the number of bytes this block compresses.
     */
    void write_block_header(uint32_t num_bytes_compressed_in_block);

    /**
     * This function gets the prefix free code and the len of that code in bits, and writes it into the buffer.
     * @param code_len the len in bits of the prefix free code to be writin into the buffer
     * @param huffman_code the huffman code to be writin into the buffer
     */
    void write_code_into_buffer(uint16_t code_len, uint64_t huffman_code);

    /**
     * This function writes into the buffer the length of each huffman code.
     * It writes it as an array of 1 byte since the huffman code is limited by 15 bits length.
     * @param tree_code_length_table this vector holds to each symbol(the index) the len of the huffman code to that symbol
     */
    void write_tree_dict(std::vector<uint16_t>& tree_code_length_table);

    /**
     * this function codes a vector that was coded using lempel ziv into binary code using huffman code
     * @param coded_vec the coded vec to code into binary.
     * @param canonial_code_1 the huffman code to use to literals and window lengths
     * @param canonial_code_2 the huffman code to use for distances
     *
     * @return the number of bytes needed to compress this vec
     */
    void write_vec_code(const ::CodedVec& coded_vec,
                        std::vector<HuffmanBuilder>& canonial_code_1
                        , std::vector<HuffmanBuilder>& canonial_code_2);

    //#################### FUNCTION TO WRITE INTO THE FILE #####################


    //############# HUFFMAN BINARY CODE HELPER FUNCTIONS########################
    /**
     * This function creates the huffman trees from the coded vec.
     * The last index of the vector holds the root of the tree.
     *
     * @return two huffman trees, ONE: codes the literal and window lengths of in the vec, TWO: codes the distances
     */
    static std::pair<huffmanTree, huffmanTree> get_huffman_trees_from_vecs(::CodedVec& coded_vec);

    /**
     *  Uses the frquency of each symbol (the first 'num_symbols' elements in the vector
     *  are the symbols and thier frequency) to create a huffman tree.
     *
     * @param tree the symbols(that are already saved as leaves) with thies frequency to copute the tree
     * @param num_symbols the number of symbols this huffman tree codes
     */
    static void create_huffman_tree(huffmanTree& tree, int num_symbols);

    /**
     * This function gets a huffman tree and computes the code length of each symbol.
     *
     * @param tree a complete huffman tree (the leaves are the first table_size values)
     * @param table_size the number of symbols the the tree coded
     * @return a vector that maps each symbol (the index in the vector) to it's code length acording to this huffman tree
     */
    static std::vector<uint16_t> get_code_len_table(huffmanTree& tree, uint32_t table_size);

    static void deflate_code_length(std::vector<uint16_t>& code_length_table);

    /**
     * This function takes a code_len_table and converts it to a huffman canonial code table
     * (symbol)->(len_of_huffman_code, huffman_code), the index is the symbol and the value at the vector is the code.
     * @param code_len_table vector that maps each symbol (the index in the vector) to it's code length that maintains the craft inequality
     * @return a maping from a symbol(the index of the table) to (len_of_huffman_code, huffman_code).
     */
    static std::vector<HuffmanBuilder> create_canonial_huffman_code(const std::vector<uint16_t>& code_len_table);


    /**
     * This function is responsible to give each symbol the number of times it apeared in the vector
     *
     * @param coded_vec the vector of symbols we want to calculate the frequency of each symbol in that vector
     * @param tree1 the first huffman tree encoding the literals and window lengths
     * @param tree2 the second huffman tree encoding the distances(the past index the window starts at).
     */
    static void add_frequency_to_symbols(CodedVec& coded_vec, huffmanTree tree1, huffmanTree tree2);

    LempelZivBlockCode decompress_block(binary_io::FileReader& file_reader);

    /**
     * this function codes into binary the the coded vec into a block and writes it into the file.
     * @param coded_vec the coded vec of this block
     * @param num_bytes_compressed_in_block the number of bytes of the original uncompressed file this block compresses
     */
    void compress_block(CodedVec& coded_vec, uint32_t num_bytes_compressed_in_block);


    //############# HUFFMAN BINARY CODE HELPER FUNCTIONS########################


    //############## DATA STRUCTURES METHODS ################
    static uint16_t get_symbol_from_range_for_tree_1(uint32_t val);

    static DistanceEncodeInfo get_symbol_from_range_for_tree_2(uint32_t val);

    /**
     * This function gets the huffman code and is responsible to create a map that will map every number with the prefix
     * of that huffman code to the symbol that that hufmman code encodes and the number of bits of that huffman code.
     *
     * @return the Decoding vector
     */
    std::vector<Decode> get_encription_table(int num_symbols);

    /**
     * This function extracts the length table from the block's code.
     * 
     * @param num_symbols the number of symbols to extract their length
     * @return a vector that maps each symbol(the index) to its prefix free code length
     */
    std::vector<uint16_t> extract_len_table_for_huffman_code(size_t num_symbols);

    /**
   * this function returns the next 'count' bits in the buffer
   * @param count the number of bits to peak ahead
   * @return the window of bits
   */
    uint32_t peak_bits_from_buffer(uint8_t count);

    /**
     *  This function gets the current symbol read that encodes a window length, and reads from the buffer the
     *  the rest of the data to decode the actual coded window length and writes it into the vector
     *  
     * @param block_code the data structure that holds the decoded vector
     * @param symbol the symbol of the window length we want to decode
     */
    void decode_window_length(LempelZivBlockCode& block_code, uint8_t symbol);

    /**
     * This function gets the current symbol read that encodes a distance and reads from the buffer  the
     * the rest of the data to decode the actual coded distance and writes it into the vector
     * @param block_code the data structure that holds the decoded vector
     * @param symbol the symbol of the distance we want to decode
     */
    void decode_distance(LempelZivBlockCode& block_code, uint8_t symbol);

    /**
     * this function advances the buffer 'num_bits' bits.
     * @param num_bits the number of buts to advance the buffer
     */
    void advance_buffer(uint8_t num_bits);

    /**
     * this function reads new data into the buffer from the file, making sure that the data that was not read yeat is
     * saved in the new buffer.
     *
     * @param file_reader the file to read the new data from
     * @param safe_end the pointer to the part in the buffer we dont want to cross
     */
    void read_data_into_buffer(binary_io::FileReader& file_reader, uint8_t*& safe_end);

    /**
     * This function fills in the field 'windowLengthToCode' which is a table that lets us access in O(1) all the
     * relevent data to encode a window lenght without needing to calculate anything.
     * The reason we have this method for window lengths and not for distances is that there are 2069 different window
     * lenghts but there can be up to 4MB of distances (oproxemetly 4 milion bytes) so having a table for that is
     * inpracticle.
     * @param canonial_code the prefix free code for each symbol
     */
    static void fill_table_windowLengthToCode(const std::vector<HuffmanBuilder>& canonial_code);

    //############## DATA STRUCTURES METHODS ################


    //#################### FIELDS ###################
    // this buffer hold the coded data before being flushed from ro to the actual file.
    std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> buffer = std::make_shared<std::array<uint8_t,BUFFER_SIZE>>();
    uint8_t* buffer_iter; // indicates what byte in buffer we are
    uint8_t offset; // holds number between 0 and 7 to indicate at what bit we are in the buffer
    uint32_t num_bytes_read_in_buffer; // the number of bytes in the buffer that has data


    // number of symbols in each huffman tree
    static const int TREE1_NUM_SYMBOLS = 285;
    static const int TREE2_NUM_SYMBOLS = 56;

    static const uint16_t WINDOW_OFFSET = 256;

    // maps from symbol of the huffman code to the range of number it represents
    static std::array<uint32_t, TREE1_NUM_SYMBOLS> tree1_symbolToRange_table;
    static std::array<uint32_t, TREE2_NUM_SYMBOLS> tree2_symbolToRange_table;

    static std::array<WindowLengthToCode, 2069> windowLengthToCode;

    static std::array<uint8_t, TREE1_NUM_SYMBOLS - 256> symbol_to_num_extra_bits_map1;
    static std::array<uint8_t, TREE2_NUM_SYMBOLS> symbol_to_num_extra_bits_map2;

    //#################### FIELDS ###################
};

#endif //HUFFMAN_CODE_H
