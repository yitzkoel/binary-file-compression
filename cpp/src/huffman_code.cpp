//
// Created by yitzk on 8/19/2026.
//

#include "../include/huffman_code.h"


void Huffman_code::compress(const std::string& file_path,
                            std::vector<::coded_vec>& coded_vecs,
                            std::vector<::bit_map>& bit_maps,
                            std::uint64_t original_file_size)
{
    void write_global_header();


    for (int i = 0; i < coded_vecs.size(); i++)
    {
        compress_block(coded_vecs[i], bit_maps[i]);
    }
}

void Huffman_code::decompress(const std::string& file_path)
{
}

void Huffman_code::clear()
{
}

void Huffman_code::compress_block(::coded_vec& coded_vec, ::bit_map& bit_map)
{
    void write_block_header();

    // get the huffman trees
    std::pair<std::shared_ptr<BinaryTreeNode>, std::shared_ptr<BinaryTreeNode>> trees =
        get_trees_from_vecs(coded_vec, bit_map);
    std::shared_ptr<BinaryTreeNode> tree1 = trees.first;
    std::shared_ptr<BinaryTreeNode> tree2 = trees.second;

    // calculate the code lenth of each symbol and put it in a table
    std::vector<uint16_t> tree1_code_length_table = get_code_len_table(tree1, TREE1_CODE_LENGTH_TABLE_SIZE);
    std::vector<uint16_t> tree2_code_length_table = get_code_len_table(tree1, TREE2_CODE_LENGTH_TABLE_SIZE);

    // get the canonial code for each tree
    std::vector<uint16_t> canonial_code_1 = create_canonial_huffman_code(tree1_code_length_table);
    std::vector<uint16_t> canonial_code_2 = create_canonial_huffman_code(tree2_code_length_table);

    // write into the buffer the code length of the huffman code
    write_tree_dict(tree1_code_length_table);
    write_tree_dict(tree2_code_length_table);

    // code the coded_vec into the buffer and from there flushed to the file
    write_vec_code(coded_vec, bit_map);

    // add end of file symbol to the end of the block (EOF)
    write_EOF();
}

void Huffman_code::write_global_header()
{
}

std::pair<std::shared_ptr<BinaryTreeNode>, std::shared_ptr<BinaryTreeNode>> Huffman_code::get_trees_from_vecs(
    ::coded_vec, ::bit_map)
{
}
