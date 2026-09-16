//
// Created by yitzk on 9/16/2026.
//

#ifndef HUFFMANCODE_H
#define HUFFMANCODE_H
#include <cstdint>
#include <vector>
#include <type_traits>

template <typename VType>
using DataVec = std::vector<VType>;

struct HuffmanTreeNode
{
    HuffmanTreeNode(uint32_t frequency, uint16_t left, uint16_t right): frequency(frequency), left(left), right(right)
    {
    }
    HuffmanTreeNode():frequency(0), left(UINT16_MAX),right(UINT16_MAX){}
 
    uint32_t frequency;
    uint16_t left;
    uint16_t right;
};

using HuffmanTree = std::vector<HuffmanTreeNode>;


struct HuffmanCode
{
    uint8_t len_code;
    uint16_t huffman_code;
};



class HuffmanBuilder {
public:
    /**
     * This is the constructor to the class
     * @param max_code_len the max code length allowed
     */
    explicit HuffmanBuilder(int max_code_len);

    HuffmanBuilder() = default;

    /**
     *
     * @tparam MapperFunc the function that maps the elements of the vector to a symbol to be coded
     * @param coded_vec the vector to create from the huffman tree
     * @param mapper the mapper function
     * @param num_symbols the number of symbols that can be mapped from the vector
     * @return A huffman tree object created from the vector
     */
    template <typename VType, typename MapperFunc>
    HuffmanTree get_huffman_tree(const DataVec<VType>& data_vec, MapperFunc mapper, int num_symbols);

    // TODO ADD to the get_huffman_tree implementation
    // static_assert(std::is_convertible_v<decltype(mapper(input_data[0])), uint32_t>,
    //                  "CRITICAL ERROR: The mapper function MUST return a uint32_t (or compatible type)!");

    /**
     * This function gets a vector of data and a mapper from the data to a symbol(a number).
     * and creates a huffman code vector that maps each symbol to its code.
     *
     * @tparam VType the type of the vector
     * @tparam MapperFunc the function that maps the vector type to a uint_32 value symbol that we encode
     * @param data_vec the vector of the data to build to the huffman code
     * @param mapper the mapper from Vtype to uint32_t
     * @param num_symbols the number of symbols that the mapper can produce (from 0 to num_symbols)
     * @return the huffman code to each symbol that apear in the vector.
     */
    template <typename VType, typename MapperFunc>
    std::vector<HuffmanCode> get_canonial_huffman_code(const DataVec<VType>& data_vec, MapperFunc mapper, int num_symbols);

    /**
     *
     * @param code_len_table a table that maps each symbol to its code lenght
     * @return the huffman code to each symbol that has lenght that is non zero.
     */
    std::vector<HuffmanCode> get_canonial_huffman_code(const std::vector<uint16_t>& code_len_table);

private:
    /**
     * This function counts the frequency of each symbol in the data vector (using the mapper to get the symbol)
     * @tparam VType the type of the vector
     * @tparam MapperFunc the function that maps the vector type to a uint_32 value symbol that we encode
     * @param tree the tree to fill with frequency(the first num_symbols are the symbols of the tree)
     * @param data_vec the data
     * @param mapper
     */
    template <typename VType, typename MapperFunc>
    void count_symbol_frequency(HuffmanTree& tree, const DataVec<VType>& data_vec, MapperFunc mapper);
    // TODO add the assert(symbol < num_symbols && "CRITICAL: Mapper returned a symbol out of bounds!"); to make sure that i didnt send bad mappper function

    /**
     * this function implements the algorithm of building the actual tree
     * @param tree the tree with the filled frequencies
     */
    void create_huffman_tree(HuffmanTree& tree);

    /**
     * thus function gets a full huffman tree and calculates the lenght of each symbols huffman code acording to that tree.
     * @param tree the tree to calc from the code lenghts
     * @param table_size the size of the table
     * @return a table that maps each symbol to its code length
     */
    static std::vector<uint16_t> get_code_len_table(HuffmanTree& tree, uint32_t table_size);

    /**
     * This function deflates the table to hold lengths that are not longer then max_code_len,
     * while maintaining the craft inequality therefore allowing to create a canonial huffman code.
     * @param code_length_table the table to deflate
     */
    static void deflate_code_length(std::vector<uint16_t>& code_length_table);

  int max_code_len = 15;
};



#endif //HUFFMANCODE_H
