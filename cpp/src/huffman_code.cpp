//
// Created by yitzk on 8/19/2026.
//

#include "../include/huffman_code.h"
// TODO implement DEFLATE algorithm

Huffman_code::Huffman_code()
{
    // setup tree1_symbolToRange_table
    uint32_t jump = 1;
    uint8_t extra_bit_len = 0;
    for (int i = 0; i < TREE1_NUM_SYMBOLS; i++)
    {
        // if we are at the literal and EOF symbols add the same value to the table
        if (i <= 255)
        {
            tree1_symbolToRange_table[i] = i;
            continue;
        }
        // if we are at the symbols that represend window lenght are from index 257 - 285 then:


        int window_len_symbol = i - 256;
        // if we are the first 18 window len symbols then we just keep the length of that symbol from 4 to 22
        if (window_len_symbol <= 18)
        {
            tree1_symbolToRange_table[i] = 4 + window_len_symbol;
            symbol_to_num_extra_bits_map1[window_len_symbol] = extra_bit_len;
        }
        // else we are at the ranges: 2, 4, 8, 16, 32, 64, 128, 265, 512, 1024 (plus the base 22) 
        else
        {
            tree1_symbolToRange_table[i] = tree1_symbolToRange_table[i - 1] + jump;
            jump = jump << 1; // mult by 2

            symbol_to_num_extra_bits_map1[window_len_symbol] = ++extra_bit_len;
        }
    }


    // setup tree2_symbolToRange_table
    jump = 1;
    extra_bit_len = 0;
    for (int i = 0; i < TREE2_NUM_SYMBOLS; i++)
    {
        if (i < 16)
        {
            tree2_symbolToRange_table[i] = i + 1;
            symbol_to_num_extra_bits_map2[i] = extra_bit_len;
        }
        else
        {
            tree2_symbolToRange_table[i] = tree2_symbolToRange_table[i - 1] + jump;
            if (i % 2 == 0)
            {
                jump = jump << 1;
                extra_bit_len++;
            }
            symbol_to_num_extra_bits_map2[i] = extra_bit_len;
        }
    }

    // TODO fill in the fields of the tables num_extra_bytes
}

void Huffman_code::flush_block(const std::string& string)
{
}

void Huffman_code::compress(const std::string& file_path,
                            std::vector<::coded_vec>& coded_vecs,
                            std::vector<::bit_map>& bit_maps,
                            std::vector<uint32_t>& num_bytes_in_block_before_compression,
                            std::uint64_t original_file_size)
{
    // init thr file_writer
    binary_io::FileWriter file_writer(file_path);
    // reset the buffer the buffer
    index_in_buffer = 0;

    // initalize the file
    write_global_header(original_file_size);

    // compress the data
    for (int i = 0; i < coded_vecs.size(); i++)
    {
        compress_block(coded_vecs[i], bit_maps[i], num_bytes_in_block_before_compression[i]);


        file_writer.flush_buffer_to_file(buffer, index_in_buffer);
        index_in_buffer = 0;
    }
}

std::vector<Decode> Huffman_code::get_encription_table(const std::vector<HuffmanCode>& huffman_code)
{
    std::vector<Decode> encription_table(2 << 15);

    // for each symbol fill in all the indexes that start with the value of the huffman code with the symbol and lenght
    // of huffman code
    for (int i = 0; i < huffman_code.size(); i++)
    {
        uint16_t offset = 1 << huffman_code[i].len_code;
        uint16_t num_iterations = 2 << (15 - huffman_code[i].len_code);
        uint16_t index_in_table = huffman_code[i].huffman_code;
        for (int j = 0; j < num_iterations; j++)
        {
            encription_table[index_in_table].symbol = i;
            encription_table[index_in_table].num_bytes = huffman_code[i].len_code;

            index_in_table += offset;
        };
    }

    return encription_table;
}


std::vector<coded_vec> Huffman_code::decompress(const std::string& file_path)
{
    std::vector<coded_vec> coded_vecs;
    std::vector<::bit_map> bit_maps;
    std::vector<uint32_t> num_bytes_compressed_in_blocks;


    binary_io::FileReader file_reader(file_path);
    file_reader.slide_window();


    buffer = file_reader.get_buffer();
    index_in_buffer = 0;
    num_bytes_read_in_buffer = file_reader.get_num_bytes_read();


    uint64_t original_file_size;
    memcpy(&original_file_size, buffer->data(), 8);
    index_in_buffer++;

    uint64_t current_original_file_bytes_read = 0;
    while (current_original_file_bytes_read < original_file_size)
    {
        LempelZivBlockCode lempelZiv_block_code = decompress_block(file_reader);
        current_original_file_bytes_read += lempelZiv_block_code.num_bytes_compressed_in_block;
    }


    return coded_vecs;
}

LempelZivBlockCode Huffman_code::decompress_block(binary_io::FileReader& file_reader)
{
    LempelZivBlockCode block_code;

    // write the number of uncompressed bytes
    memcpy(&block_code.num_bytes_compressed_in_block, buffer->data() + index_in_buffer, 3);
    index_in_buffer += 3;

    // write the number of bytes the whole block takes
    uint32_t compressed_size; // number of bytes in the compressed block
    memcpy(&compressed_size, buffer->data() + index_in_buffer, 3);
    index_in_buffer += 3;

    // get the code lengths of each symbol and put it in a table
    std::vector<uint16_t> tree1_code_length_table = extract_len_table_for_huffman_code();
    std::vector<uint16_t> tree2_code_length_table = extract_len_table_for_huffman_code();

    // get the canonial code for each tree
    // map from symbol( the index of the vector to (len,binary_code)
    std::vector<HuffmanCode> canonial_code_1 = create_canonial_huffman_code(tree1_code_length_table);
    std::vector<HuffmanCode> canonial_code_2 = create_canonial_huffman_code(tree2_code_length_table);

    std::vector<Decode> encryption_bytes_to_symbol_1 = get_encription_table(canonial_code_1);
    std::vector<Decode> encryption_bytes_to_symbol_2 = get_encription_table(canonial_code_2);


    // TODO should i release unused resources
    uint32_t index_in_block = 0;
    index_in_buffer = 64;


    while (index_in_block < compressed_size)
    {
        // dealing with updating the buffers
        uint32_t num_bytes_deciphered_from_buffer = index_in_buffer - 8 + (index_in_byte_buffer % 8);
        if (num_bytes_deciphered_from_buffer >= num_bytes_read_in_buffer - 8)
        {
            // move remaining buffer into eight_bytes_buffer
            eight_bytes_buffer = eight_bytes_buffer >> index_in_byte_buffer;

            uint64_t remaining_bytes;
            memcpy(&remaining_bytes, buffer->data() + index_in_buffer, num_bytes_read_in_buffer - index_in_buffer);

            remaining_bytes = remaining_bytes << index_in_byte_buffer;

            eight_bytes_buffer = eight_bytes_buffer | remaining_bytes;
            index_in_byte_buffer = 0;

            file_reader.slide_window();
            buffer = file_reader.get_buffer();
            index_in_buffer = 0;
            num_bytes_read_in_buffer = file_reader.get_num_bytes_read();
        }

        // TODO update  eight_bytes_buffer

         uint64_t code = eight_bytes_buffer >> index_in_byte_buffer;
         code = code & first_15_bytes_mask;
        index_in_byte_buffer += encryption_bytes_to_symbol_1[code].num_bytes;



        // we encoded a window length
        if(encryption_bytes_to_symbol_1[code].symbol >= 256)
        {
          uint64_t extra_val  = eight_bytes_buffer >> index_in_buffer;
            extra_val = extra_val & (1u << symbol_to_num_extra_bits_map1[encryption_bytes_to_symbol_1[code].symbol]) -1;
            block_code.coded_vec.push_back(tree1_symbolToRange_table[encryption_bytes_to_symbol_1[code].symbol] + extra_val);

            index_in_byte_buffer += symbol_to_num_extra_bits_map1[encryption_bytes_to_symbol_1[code].symbol];

            // we encode a

        }

        else
        {
            block_code.coded_vec.push_back(encryption_bytes_to_symbol_1[code].symbol);
        }




    }


    return block_code;
}

void Huffman_code::clear()
{
}


void Huffman_code::compress_block(::coded_vec& coded_vec, ::bit_map& bit_map,
                                  uint32_t num_bytes_in_block_before_compression)
{
    // write the bolcj header int the buffer
    write_block_header(num_bytes_in_block_before_compression);

    // advance the buffer count to reserve space to the number of bytes the copressed vector took
    index_in_buffer += 3;

    // get the huffman trees
    std::pair<std::vector<HuffmanTreeNode>, std::vector<HuffmanTreeNode>> trees =
        get_huffman_trees_from_vecs(coded_vec, bit_map);
    std::vector<HuffmanTreeNode> tree1 = trees.first;
    std::vector<HuffmanTreeNode> tree2 = trees.second;

    // calculate the code lengths of each symbol and put it in a table
    std::vector<uint16_t> tree1_code_length_table = get_code_len_table(tree1, TREE1_NUM_SYMBOLS);
    std::vector<uint16_t> tree2_code_length_table = get_code_len_table(tree2, TREE2_NUM_SYMBOLS);

    // TODO should i release unused resources like the tree1_code_length_table  and tree1?

    // get the canonial code for each tree
    // map from symbol( the index of the vector to (len,binary_code)
    std::vector<HuffmanCode> canonial_code_1 = create_canonial_huffman_code(tree1_code_length_table);
    std::vector<HuffmanCode> canonial_code_2 = create_canonial_huffman_code(tree2_code_length_table);


    // fill the table windowLengthToCode
    fill_table_windowLengthToCode(canonial_code_1);


    // write into the buffer the code lengths of the huffman code
    write_tree_dict(tree1_code_length_table);
    write_tree_dict(tree2_code_length_table);


    // code the coded_vec into the buffer and from there flushed to the file
    write_vec_code(coded_vec, bit_map, canonial_code_1, canonial_code_2);

    // write the number of compressed bytes that vector took
    write_number_of_compressed_bytes(index_in_buffer - 6);
}

void Huffman_code::write_global_header(std::uint64_t original_file_size)
{
    // copy the suze of the file to the begining of the buffer
    memcpy(buffer->data(), &original_file_size, 8);
    index_in_buffer += 8;
}

void Huffman_code::write_tree_dict(std::vector<uint16_t>& tree_code_length_table)
{
    for (int i = 0; i < tree_code_length_table.size(); i++, index_in_buffer++)
    {
        (*buffer)[index_in_buffer] = tree_code_length_table[i];
    }
}


void Huffman_code::write_block_header(uint32_t num_bytes_compressed_in_block)
{
    memcpy(buffer->data() + index_in_buffer, &num_bytes_compressed_in_block, 3);
    index_in_buffer += 3;
}

void Huffman_code::write_number_of_compressed_bytes(uint32_t number_of_compressed_bytes)
{
    memcpy(buffer->data() + 3, &number_of_compressed_bytes, 3);
}


void Huffman_code::write_vec_code(const ::coded_vec& coded_vec, const ::bit_map& bit_map,
                                  std::vector<HuffmanCode>& canonial_code_1,
                                  std::vector<HuffmanCode>& canonial_code_2)
{
    // setting up the bitmap indexes and current buffer
    uint32_t bit_map_index = 0;
    uint64_t bit_mask = 1;
    uint64_t cur_bit_map = bit_map[bit_map_index];

    // init the buffer's buffer
    eight_bytes_buffer = 0;
    index_in_byte_buffer = 0;
    for (uint32_t i = 0; i < coded_vec.size(); i++)
    {
        if (bit_mask == 0)
        {
            bit_mask = 1;
            bit_map_index++;
            cur_bit_map = bit_map[bit_map_index];
        }
        uint32_t val = coded_vec[i];
        // val is a literal
        if ((cur_bit_map & bit_mask) == 0)
        {
            // getting the len and code of the huffman code of 'val'
            uint16_t code_len = canonial_code_1[val].len_code;
            uint64_t huffman_code = canonial_code_1[val].huffman_code;
            write_code_into_buffer(code_len, huffman_code);
        }
        // val is a window len
        else
        {
            // write the code for window len into the buffer
            write_code_into_buffer(windowLengthToCode[val].huffman_len, windowLengthToCode[val].huffman_code);
            write_code_into_buffer(windowLengthToCode[val].extra_bits_len, windowLengthToCode[val].extra_bits_val);

            // write the code for 'Distance' into the buffer

            // get the next val
            val = coded_vec[++i];

            // get the distance info (that is the symbol in the tree, the extra bits, and the len of the diffarance
            DistanceEncodeInfo dist_info = get_symbol_from_range_for_tree_2(val);
            uint16_t huffman_code = canonial_code_2[dist_info.symbol].huffman_code;
            uint8_t code_len = canonial_code_2[dist_info.symbol].len_code;

            // write the data into the buffer
            write_code_into_buffer(code_len, huffman_code);
            write_code_into_buffer(dist_info.extra_bits_len, dist_info.extra_bits_val);
        }
        bit_mask = bit_mask << 1;
    }
}


void Huffman_code::write_code_into_buffer(uint16_t code_len, uint64_t huffman_code)
{
    uint8_t num_bits_left_in_byte_buffer = 64 - index_in_byte_buffer;

    // writing into the space that is left in the buffer the code
    uint64_t write_1 = huffman_code << index_in_byte_buffer;
    eight_bytes_buffer = eight_bytes_buffer | write_1;

    uint8_t num_bits_read = std::min<uint16_t>(code_len, num_bits_left_in_byte_buffer);
    index_in_byte_buffer += num_bits_read;

    // flushing the eight_bytes_buffer into the file buffer
    if (index_in_byte_buffer == 64)
    {
        memcpy(buffer->data() + index_in_buffer, &eight_bytes_buffer, 8);
        index_in_buffer += 8;
        index_in_byte_buffer = 0;
        eight_bytes_buffer = 0;
    }
    // writing what is left of the code into the buffer
    uint64_t write_2 = huffman_code >> num_bits_read;
    eight_bytes_buffer = eight_bytes_buffer | write_2;
}


void Huffman_code::add_frequency_to_symbols(coded_vec& coded_vec, bit_map& bit_map, std::vector<HuffmanTreeNode> tree1,
                                            std::vector<HuffmanTreeNode> tree2)
{
    // calculate the frequency of each symbol
    int bit_map_index = 0;
    uint64_t bit_mask = 1;
    uint64_t cur_bit_map = bit_map[bit_map_index];
    for (uint32_t i = 0; i < coded_vec.size(); i++)
    {
        // update the bit map if we reched the end of cur_bit_map
        if (bit_mask == 0)
        {
            bit_mask = 1;
            bit_map_index++;
            cur_bit_map = bit_map[bit_map_index];
        }

        uint32_t val = coded_vec[i];

        // if val is a literal
        if ((cur_bit_map & bit_mask) != 0)
        {
            tree1[val].frequency++;
        }
        // else val is a window length
        else
        {
            // calculate the symbol that this window lenght falls in that range
            tree1[get_symbol_from_range_for_tree_1(val)].frequency++;

            // advance the index of the vector to get the distance
            i++;

            // val is distance
            val = coded_vec[i];
            tree2[get_symbol_from_range_for_tree_2(val).symbol].frequency++;
        }
        // advance the bit mask via shift
        bit_mask = bit_mask << 1;
    }
}

std::pair<std::vector<HuffmanTreeNode>, std::vector<HuffmanTreeNode>>
Huffman_code::get_huffman_trees_from_vecs(::coded_vec& coded_vec, ::bit_map& bit_map)
{
    // reserve all nodes of the huffman trees
    std::vector<HuffmanTreeNode> tree1;
    std::vector<HuffmanTreeNode> tree2;
    tree1.reserve(TREE1_NUM_SYMBOLS * 2 - 1);
    tree2.reserve(TREE2_NUM_SYMBOLS * 2 - 1);


    // fill in the nodes of the leafs
    fill_leaf_nodes(tree1);
    fill_leaf_nodes(tree2);

    // calculate the frequency of each symbol and add it to the apropriate tree
    add_frequency_to_symbols(coded_vec, bit_map, tree1, tree2);

    // create the actual huffman tree using the calculated frequencies
    create_huffman_tree(tree1, TREE1_NUM_SYMBOLS);
    create_huffman_tree(tree2, TREE2_NUM_SYMBOLS);

    return std::pair{tree1, tree2};
}

void Huffman_code::create_huffman_tree(std::vector<HuffmanTreeNode>& tree, int num_symbols)
{
    // create min heap with the indexes of the tree with a custom compare function so the min heap will extract the min frequency
    auto cmp = [&tree](int left_index, int right_index)
    {
        return tree[left_index].frequency > tree[right_index].frequency;
    };

    // this queue holds the symbol as its value and the queue is determined by the frequency of that symbol.
    std::priority_queue<int, std::vector<int>, decltype(cmp)> min_heap_tree(cmp);

    // add to the min heap all symbols that have frequency bigger than 0.
    for (int i = 0; i < num_symbols; i++)
    {
        if (tree[i].frequency > 0) min_heap_tree.push(i);
    }

    // build the huffman tree
    int i = num_symbols;
    while (!min_heap_tree.empty())
    {
        // get the two smallest frequency nodes
        int node1 = min_heap_tree.top();
        min_heap_tree.pop();
        int node2 = min_heap_tree.top();
        min_heap_tree.pop();

        // merge node 1 and node 2 to a new node with the combined frequency
        // and add node 1 and node 2 as its children.
        uint32_t new_frequency = tree[node1].frequency + tree[node2].frequency;
        tree.emplace_back();
        tree[i].frequency = new_frequency;
        tree[i].left = node1;
        tree[i].right = node2;
        i++;
    }
}

std::vector<uint16_t> Huffman_code::get_code_len_table(std::vector<HuffmanTreeNode>& tree, uint32_t table_size)
{
    // create len_table with all the symbols with len 0.
    std::vector<uint16_t> len_table(table_size, 0);

    //********* Use BFS to travers the tree to get all the code lengths *********

    // FIFO data structure that hold the node and its depth in the tree
    std::deque<std::pair<int, int>> deque;

    // give the root depth 0
    deque.emplace_back(tree.size() - 1, 0);

    // BFS the tree
    while (!deque.empty())
    {
        // get the top node and its depth
        int node = deque.front().first;
        int depth = deque.front().second;
        deque.pop_back();

        // if the node is a symbol then we reached a leaf, so we can update its length
        if (node < table_size) len_table[node] = depth;

        // else we are at a intersection and we need to keep tranversing the tree
        else
        {
            if (tree[node].left == -1) deque.emplace_back(tree[node].left, depth + 1);
            if (tree[node].right == -1) deque.emplace_back(tree[node].right, depth + 1);
        }
    }
    return len_table;
}

// this method assumes the code len is not biggier than 15 bit
std::vector<HuffmanCode> Huffman_code::create_canonial_huffman_code(
    const std::vector<uint16_t>& code_len_table)
{
    // the index is the symbol and the table maps from the index(symbol) to the len of the code and the code(in 64 bits)
    std::vector<HuffmanCode> canonial_code(code_len_table.size(), {0, 0});

    // create a vetor from symbol to len
    std::vector<std::pair<uint16_t, uint16_t>> symbol_to_len;
    for (int i = 0; i < code_len_table.size(); i++)
    {
        if (code_len_table[i] > 0) symbol_to_len.emplace_back(i, code_len_table[i]);
    }

    // sort the symbol to len by len then by symbol
    std::sort(symbol_to_len.begin(), symbol_to_len.end(), [](const auto& a, const auto& b)
    {
        if (a.second != b.second)
        {
            return a.second < b.second;
        }
        return a.first < b.first;
    });

    int pre_len = 0;
    uint16_t huffman_code = 0;
    for (auto symbol_to_len_pair : symbol_to_len)
    {
        if (pre_len < symbol_to_len_pair.second)
        {
            huffman_code = huffman_code << (symbol_to_len_pair.second - pre_len);
            pre_len = symbol_to_len_pair.second;
        }
        canonial_code[symbol_to_len_pair.first].len_code = symbol_to_len_pair.second;
        canonial_code[symbol_to_len_pair.first].len_code = huffman_code;
        huffman_code++;
    }

    return canonial_code;
}


void Huffman_code::fill_leaf_nodes(std::vector<HuffmanTreeNode> tree1)
{
    for (int i = 0; i < TREE1_NUM_SYMBOLS; i++)
    {
        tree1.emplace_back();
    }
}

uint16_t Huffman_code::get_symbol_from_range_for_tree_1(uint32_t val)
{
    if (val <= 18 + 4) return val - 4 + 257;

    val = val - 22;
    uint32_t msb = std::__bit_width(val) - 1;
    return msb + 18;
}

DistanceEncodeInfo Huffman_code::get_symbol_from_range_for_tree_2(uint32_t val)
{
    // base case no range in table
    if (val <= 16)
    {
        return {val - 1, 0, 0};
    }
    val = val - 13;
    uint32_t msb = std::__bit_width(val) - 1;

    uint8_t b = (val >> (msb - 1)) & 1;

    DistanceEncodeInfo info;
    info.symbol = (((msb - 2) * 2) + 16) + b;
    info.extra_bits_len = msb - 1;

    uint32_t mask = (1 << info.extra_bits_len) - 1;
    info.extra_bits_val = val & mask;

    return info;
}


void Huffman_code::fill_table_windowLengthToCode(
    const std::vector<HuffmanCode>& canonial_code)
{
    for (int i = 0; i < windowLengthToCode.size(); i++)
    {
        int symbol = get_symbol_from_range_for_tree_1(i + 4);
        uint32_t extra_bit_val = (i + 4) - tree1_symbolToRange_table[symbol];

        windowLengthToCode[i].huffman_code = canonial_code[symbol].huffman_code;
        windowLengthToCode[i].huffman_len = canonial_code[symbol].len_code;
        windowLengthToCode[i].extra_bits_val = extra_bit_val;
        windowLengthToCode[i].extra_bits_len = std::__bit_width(extra_bit_val);
    }
}
