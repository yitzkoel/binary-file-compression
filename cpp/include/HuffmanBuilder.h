//
// Created by yitzk on 9/16/2026.
//

#ifndef HUFFMANCODE_H
#define HUFFMANCODE_H
#include <cassert>
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
    uint32_t huffman_code;
};




/**
 * @brief create huffman code of max length 32 bits from a generic vector.
 *
 * This class gets a generic data vector with a mapper function that maps each data elemnt in the vector to a uint32_t
 * value, and created a binary prefix free huffan code from those symbols.
 * The entire class is static therfore you just need to insert the parameters and you get the code.
 */
class HuffmanBuilder
{
public:
 HuffmanBuilder() = default;

 /**
  *
  * @tparam MapperFunc the function that maps the elements of the vector to a symbol to be coded
  * @param data_vec the vector to create from the huffman tree
  * @param mapper the mapper function
  * @param num_symbols the number of symbols that can be mapped from the vector
  * @return A huffman tree object created from the vector
  */
 template <typename VType, typename MapperFunc>
 static  HuffmanTree get_huffman_tree(const DataVec<VType>& data_vec, MapperFunc mapper, int num_symbols)
 {
  // assert to make sure the mapper is a legal function
  static_assert(std::is_convertible_v<decltype(mapper(data_vec[0])), uint32_t>,
                   "CRITICAL ERROR: The mapper function MUST return a uint32_t (or compatible type)!");

  // init the tree with the maximum amount of possible nodes
  HuffmanTree tree((num_symbols * 2) - 1);

  // count to each symbol the number of frequencies
  count_symbol_frequency(tree, data_vec, mapper, num_symbols);

  create_huffman_tree(tree,num_symbols);

  return tree;
 }


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
 static std::vector<HuffmanCode> get_canonial_huffman_code(const DataVec<VType>& data_vec, MapperFunc mapper, int num_symbols, int max_code_len)
 {
  assert(max_code_len <= 32 && "canoot create huffman code with max code lenght longer than 32");

  HuffmanTree tree = get_huffman_tree(data_vec, mapper,num_symbols);

  std::vector<uint16_t> code_len_table = get_code_len_table(tree, num_symbols);

  deflate_code_length(code_len_table, max_code_len);

  return create_canonial_huffman_code(code_len_table);
 }

 /**
  *
  * @param code_len_table a table that maps each symbol to its code lenght
  * @return the huffman code to each symbol that has lenght that is non zero.
  */
 static std::vector<HuffmanCode> get_canonial_huffman_code(const std::vector<uint16_t>& code_len_table, int max_code_len);

 /**
  * this function returns a code len table to a huffman code table.
  * @param huffman_code the huffman code to get the code len table for
  * @return the code len table
  */
 static std::vector<uint8_t> get_code_len_table(std::vector<HuffmanCode>& huffman_code);



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
 static void count_symbol_frequency(HuffmanTree& tree, const DataVec<VType>& data_vec, MapperFunc mapper,int num_symbols)
 {
  // calculate the frequency of each symbol
  for (uint32_t i = 0; i < data_vec.size(); i++)
  {
   uint32_t symbol = mapper(data_vec[i]);
   assert(symbol < num_symbols && "CRITICAL: Mapper returned a symbol out of bounds!");

   tree[symbol].frequency++;
  }
 }

 /**
  * this function implements the algorithm of building the actual tree
  * @param tree the tree with the filled frequencies
  */
 static void create_huffman_tree(HuffmanTree& tree, int num_symbols);

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
  static void deflate_code_length(std::vector<uint16_t>& code_length_table, int max_code_len);

 /**
  * this function uses the canonial huffman code construction to build the a huffman code for the lengths in the len table.
  * Please note that the len table need to satisfy the kraft inequality and that this method does not verify that for
  * performance reasons.
  *
  * @param code_len_table the code len table to calculate the canonial huffman code from
  * @return the huffman code
  */
 [[nodiscard]] static std::vector<HuffmanCode> create_canonial_huffman_code(const std::vector<uint16_t>& code_len_table) ;
};

#endif //HUFFMANCODE_H
