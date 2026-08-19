//
// Created by yitzk on 8/19/2026.
//

#ifndef HUFFMAN_CODE_H
#define HUFFMAN_CODE_H

#include <memory>
#include <string>
#include <vector>
#include <..\include\binary_io.h>




class BinaryTreeNode
{
public:
    explicit BinaryTreeNode(uint32_t symbol): symbol(symbol)
    {
    }

    uint32_t symbol;
    uint32_t frequency = 0;
    std::unique_ptr<BinaryTreeNode> right = nullptr;
    std::unique_ptr<BinaryTreeNode> left = nullptr;

private:
};


using coded_vec = std::vector<uint32_t>;
using bit_map = std::vector<uint64_t>;


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
 *
 * <Tree 1 Definition (Literals & Lengths)>
 *      <num_symbols_1>  -> 2 bytes. Number of symbols in the first tree (max 286).
 *      <bit_lengths_1>  -> Array of lengths used to reconstruct the Canonical Huffman tree.
 *                          (Symbol values:
                                             0-255:   Literals,
                                             256:     EOF_BLOCK,
                                             257-285: Length Ranges).
 *
 * <Tree 2 Definition (Distances)>
 *      <num_symbols_2>  -> 2 bytes. Number of symbols in the second tree (e.g., 44 for 4MB window).
 *      <bit_lengths_2>  -> Array of lengths used to reconstruct the Canonical Huffman tree.
 *                          (Symbol values represent Distance Ranges).
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
    explicit Huffman_code();

    void compress(const std::string& file_path,
                  std::vector<coded_vec>& coded_vecs,
                  std::vector<bit_map>& bit_maps,
                  std::uint64_t original_file_size);

    void decompress(const std::string& file_path);

    void clear();

private:
    std::vector<uint16_t> get_code_len_table(const std::shared_ptr<BinaryTreeNode>& tree1, int table_size);
    std::vector<uint16_t> create_canonial_huffman_code(const std::vector<uint16_t>& vector);
    void write_tree_dict(const std::vector<uint16_t>& vector);
    void write_vec_code(const ::coded_vec& codeds, const ::bit_map& vector);
    void write_EOF();
    void compress_block(coded_vec& coded_vec, bit_map& bit_map);
    void write_global_header();
    std::pair<std::shared_ptr<BinaryTreeNode>,std::shared_ptr<BinaryTreeNode>>
    get_trees_from_vecs(coded_vec, bit_map);


    std::shared_ptr<std::array<uint8_t,BUFFER_SIZE>> buffer =
        std::make_shared<std::array<uint8_t,BUFFER_SIZE>>();

    std::shared_ptr<std::vector<uint32_t>> coded_vec;
    std::shared_ptr<std::vector<uint64_t>> bit_map;

static const int TREE1_CODE_LENGTH_TABLE_SIZE = 286;
static const int TREE2_CODE_LENGTH_TABLE_SIZE = 44;
};

#endif //HUFFMAN_CODE_H
