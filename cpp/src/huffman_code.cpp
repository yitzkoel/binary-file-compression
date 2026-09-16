//
// Created by yitzk on 8/19/2026.
//

#include "../include/huffman_code.h"

// TODO implemetnt securaty mesures for example prevent ZIP bomb

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

void Huffman_code::compress(const std::string& file_path,
                            std::vector<::CodedVec>& coded_vecs,
                            std::vector<uint32_t>& num_bytes_in_block_before_compression,
                            std::uint64_t original_file_size)
{
    // init the file_writer
    binary_io::FileWriter file_writer(file_path);
    // reset the buffer
    buffer_iter = buffer->data();
    offset = 0;

    // initalize the file
    write_global_header(original_file_size);

    // compress the data
    for (int i = 0; i < coded_vecs.size(); i++)
    {
        compress_block(coded_vecs[i], num_bytes_in_block_before_compression[i]);

        // flush the block onto the file and clear the buffer
        file_writer.flush_buffer_to_file(buffer, (buffer_iter - buffer->data()) / sizeof(*buffer_iter));
        buffer_iter = buffer->data();
        offset = 0;
    }
}

std::vector<CodedVec> Huffman_code::decompress(const std::string& file_path)
{
    std::vector<CodedVec> coded_vecs;
    std::vector<uint32_t> num_bytes_compressed_in_blocks;

    // the file that the compressed data is in.
    binary_io::FileReader file_reader(file_path);

    // read from the file the first 4MB(or less if the file is smaller
    file_reader.slide_window();

    // get the data and save a pointer to it for easier handel
    buffer = file_reader.get_buffer();
    buffer_iter = buffer->data();
    offset = 0;
    num_bytes_read_in_buffer = file_reader.get_num_bytes_read();

    // first 8 bytes hold the original file size
    uint64_t original_file_size = *((uint64_t*)buffer_iter);
    buffer_iter += 8;

    uint64_t current_original_file_bytes_read = 0;
    // this var tracks the number of bytes in the original file we uncompressed so far
    while (current_original_file_bytes_read < original_file_size)
    {
        LempelZivBlockCode lempelZiv_block_code = decompress_block(file_reader);
        current_original_file_bytes_read += lempelZiv_block_code.num_bytes_compressed_in_block;

        coded_vecs.push_back(lempelZiv_block_code.coded_vec_);
    }

    return coded_vecs;
}

void Huffman_code::clear()
{
}


uint32_t Huffman_code::peak_bits_from_buffer(uint8_t count)
{
    uint64_t window = *(uint64_t*)buffer_iter; // get a window of size 64 bytes from the

    window = window >> offset;

    return window & ((1U << count) - 1);
}

void Huffman_code::decode_window_length(LempelZivBlockCode& block_code, uint8_t symbol)
{
    uint32_t extra_val = peak_bits_from_buffer(symbol_to_num_extra_bits_map1[symbol]);
    advance_buffer(symbol_to_num_extra_bits_map1[symbol]);
    uint32_t val = extra_val + tree1_symbolToRange_table[symbol];

    block_code.coded_vec_.push_back(val);
}

void Huffman_code::decode_distance(LempelZivBlockCode& block_code, uint8_t symbol)
{
    uint32_t extra_val = peak_bits_from_buffer(symbol_to_num_extra_bits_map2[symbol]);
    advance_buffer(symbol_to_num_extra_bits_map2[symbol]);
    uint32_t val = extra_val + tree2_symbolToRange_table[symbol];

    block_code.coded_vec_.push_back(val);
}

LempelZivBlockCode Huffman_code::decompress_block(binary_io::FileReader& file_reader)
{
    // TODO do i need the whole block code? not only the coded vectors?
    LempelZivBlockCode block_code;

    // write the number of uncompressed bytes this block encodes
    memcpy(&block_code.num_bytes_compressed_in_block, buffer_iter, 3);
    buffer_iter += 3;

    // write the number of bytes the trees encoding and huffman code of the vector takes
    uint32_t compressed_size;
    memcpy(&compressed_size, buffer_iter, 3);
    buffer_iter += 3;

    // create maps that map from 15 bits to the symbol that it's huffman code start with those 15 bits
    std::vector<Decode> encryption_fifteen_bits_to_symbol_1 = get_encription_table(TREE1_NUM_SYMBOLS);
    std::vector<Decode> encryption_fifteen_bits_to_symbol_2 = get_encription_table(TREE2_NUM_SYMBOLS);


    // TODO should i release unused resources?


    uint32_t index_in_block = 0; // current byte in the compressed block

    // a pointer to the end of the buffer ment to prevent overflowed reading or reading unread bytes
    // it indicates when we need to read more data from the file into the buffer
    uint8_t* safe_end = buffer->data() + num_bytes_read_in_buffer - 8;

    while (index_in_block < compressed_size)
    {
        // if we have reached the end of the buffer we need to load more bytes from the file into it
        if (safe_end < buffer_iter)
        {
            read_data_into_buffer(file_reader, safe_end);
        }

        // get index in buffer previos to reading from it to calculate the number of bytes read
        uint32_t pre_index_in_buffer = (buffer->data() - buffer_iter) / (sizeof(*buffer->data()));

        // get the next 15 bits in the buffer
        uint16_t next_chunk = peak_bits_from_buffer(15);
        advance_buffer(encryption_fifteen_bits_to_symbol_1[next_chunk].num_bits);
        uint8_t symbol = encryption_fifteen_bits_to_symbol_1[next_chunk].symbol;

        if (symbol < 256)
        {
            block_code.coded_vec_.push_back(symbol);
        }

        else
        {
            // we decode window lenght
            decode_window_length(block_code, symbol);

            // next we encode distance
            next_chunk = peak_bits_from_buffer(15);
            advance_buffer(encryption_fifteen_bits_to_symbol_2[next_chunk].num_bits);
            symbol = encryption_fifteen_bits_to_symbol_2[next_chunk].symbol;

            decode_distance(block_code, symbol);
        }

        // get index in buffer after reading from it
        uint32_t post_index_in_buffer = (buffer->data() - buffer_iter) / (sizeof(*buffer->data()));

        // update the index in the block
        index_in_block += (post_index_in_buffer - pre_index_in_buffer);
    }


    return block_code;
}

std::vector<uint16_t> Huffman_code::extract_len_table_for_huffman_code(size_t num_symbols)
{
    // the vector to put the lengths in
    std::vector<uint16_t> code_len_table;

    // iterate to fill the vector with the code lengths
    for (size_t i = 0; i < num_symbols; i++, buffer_iter++)
    {
        code_len_table.push_back(*buffer_iter);
    }

    return code_len_table;
}

std::vector<Decode> Huffman_code::get_encription_table(const int num_symbols)
{
    // get the code lengths of each symbol and put it in a table
    std::vector<uint16_t> tree_code_length_table = extract_len_table_for_huffman_code(num_symbols);

    // get the canonial code for each tree
    // map from symbol(index of the vector) to (len,binary_code)
    std::vector<HuffmanCode> canonial_code = create_canonial_huffman_code(tree_code_length_table);


    std::vector<Decode> encription_table(2 << 15);

    // for each symbol fill in all the indexes that start with the value of the huffman code with the symbol and length
    // of that symbol's huffman code
    for (int i = 0; i < canonial_code.size(); i++)
    {
        // the offset between each index of the table with the binary prefix of huffman_code[i].huffman_code
        // for example suppose the huffman code 011 so the next value that strats with 011 is 1011 which is 1000 + 011.
        uint16_t offset = 1 << canonial_code[i].len_code;

        // the number of values in the encription_table with the prefix huffman_code[i].huffman_code
        uint16_t num_iterations = 2 << (15 - canonial_code[i].len_code);

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

void Huffman_code::read_data_into_buffer(binary_io::FileReader& file_reader, uint8_t*& safe_end)
{
    // copy the last 8 bytes to the front of the buffer
    memcpy(buffer->data(), safe_end, 8);

    // have buffer iter point to the first byte in buffer with data that was not yet consumed
    // TODO maby change 'safe_end' to 'num_bytes_read_in_buffer' field and then this function is more general
    buffer_iter = buffer->data() + (buffer_iter - safe_end);

    // fill in the buffer to the max from the first 8 bytes and update the fields
    file_reader.read_bytes(buffer->data() + 8, BUFFER_SIZE - 8);
    num_bytes_read_in_buffer = file_reader.get_num_bytes_read();
    safe_end = buffer->data() + +num_bytes_read_in_buffer - 8;
}


void Huffman_code::advance_buffer(uint8_t num_bits)
{
    offset += num_bits;
    buffer_iter += offset >> 3; // devide offset by 8 and add to the iter.
    offset = offset &= 7; // modolo 8
}


void Huffman_code::compress_block(::CodedVec& coded_vec, uint32_t num_bytes_in_block_before_compression)
{
    // write the bolck header into the buffer
    write_block_header(num_bytes_in_block_before_compression);

    // advance the buffer count to reserve space to the number of bytes the copressed vector took
    uint8_t* compressed_size_iter = buffer_iter;
    buffer_iter += 3;


    // get the huffman trees
    std::pair<huffmanTree, huffmanTree> trees = get_huffman_trees_from_vecs(coded_vec);
    std::vector<HuffmanTreeNode> tree1 = trees.first;
    std::vector<HuffmanTreeNode> tree2 = trees.second;

    // calculate the code lengths of each symbol and put it in a table
    std::vector<uint16_t> tree1_code_length_table = get_code_len_table(tree1, TREE1_NUM_SYMBOLS);
    std::vector<uint16_t> tree2_code_length_table = get_code_len_table(tree2, TREE2_NUM_SYMBOLS);

    deflate_code_length(tree1_code_length_table);
    deflate_code_length(tree2_code_length_table);

    // TODO should i release unused resources like the tree1_code_length_table  and tree1?

    // get the canonial code for each tree
    // map from symbol( the index of the vector to (len,binary_code)
    std::vector<HuffmanCode> canonial_code_1 = create_canonial_huffman_code(tree1_code_length_table);
    std::vector<HuffmanCode> canonial_code_2 = create_canonial_huffman_code(tree2_code_length_table);


    // fill the table WindowLengthToCode
    fill_table_windowLengthToCode(canonial_code_1);


    // write into the buffer the code lengths of the huffman code
    write_tree_dict(tree1_code_length_table);
    write_tree_dict(tree2_code_length_table);


    // code the coded_vec_ into the buffer and from there flushed to the file
    write_vec_code(coded_vec, canonial_code_1, canonial_code_2);

    // write the number of compressed bytes that vector took
    int num_bytes_used = (buffer_iter - buffer->data()) / sizeof(*buffer_iter);
    memcpy(compressed_size_iter, (const char*)(&num_bytes_used), 3);
}

void Huffman_code::write_global_header(std::uint64_t original_file_size)
{
    // copy the size of the file to the begining of the buffer
    memcpy(buffer_iter, &original_file_size, 8);
    buffer_iter += 8;
}

void Huffman_code::write_tree_dict(std::vector<uint16_t>& tree_code_length_table)
{
    for (int i = 0; i < tree_code_length_table.size(); i++, buffer_iter++)
    {
        *buffer_iter = tree_code_length_table[i];
    }
}


void Huffman_code::write_block_header(uint32_t num_bytes_compressed_in_block)
{
    memcpy(buffer_iter, &num_bytes_compressed_in_block, 3);
    buffer_iter += 3;
}


void Huffman_code::write_vec_code(const ::CodedVec& coded_vec,
                                  std::vector<HuffmanCode>& canonial_code_1,
                                  std::vector<HuffmanCode>& canonial_code_2)
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
            uint16_t code_len = canonial_code_1[val].len_code;
            uint64_t huffman_code = canonial_code_1[val].huffman_code;
            write_code_into_buffer(code_len, huffman_code);
        }

        // else val is a window len
        else
        {
            // write the code for window len into the buffer
            uint32_t window_symbol = val - WINDOW_OFFSET;
            write_code_into_buffer(windowLengthToCode[window_symbol].huffman_len,
                                   windowLengthToCode[window_symbol].huffman_code);
            write_code_into_buffer(windowLengthToCode[window_symbol].extra_bits_len,
                                   windowLengthToCode[window_symbol].extra_bits_val);

            // write the code for 'Distance' into the buffer

            // get the next val which is distance
            val = coded_vec[++i];

            // get the distance info (the symbol in the tree, the extra bits, and the len of the diffarance
            DistanceEncodeInfo dist_info = get_symbol_from_range_for_tree_2(val);
            uint16_t huffman_code = canonial_code_2[dist_info.symbol].huffman_code;
            uint8_t code_len = canonial_code_2[dist_info.symbol].len_code;

            // write the data into the buffer
            write_code_into_buffer(code_len, huffman_code);
            write_code_into_buffer(dist_info.extra_bits_len, dist_info.extra_bits_val);
        }
    }
}


void Huffman_code::write_code_into_buffer(uint16_t code_len, uint64_t huffman_code)
{
    uint32_t window = 0;

    memcpy(&window, buffer_iter, 4);

    window &= (1ULL << offset) - 1;

    huffman_code = huffman_code << offset;

    window = window | huffman_code;

    std::memcpy(buffer_iter, &window, sizeof(window));

    advance_buffer(code_len);
}


std::pair<huffmanTree, huffmanTree> Huffman_code::get_huffman_trees_from_vecs(::CodedVec& coded_vec)
{
    // reserve all posible nodes of the huffman trees
    huffmanTree tree1(TREE1_NUM_SYMBOLS * 2 - 1);
    huffmanTree tree2(TREE2_NUM_SYMBOLS * 2 - 1);

    // calculate the frequency of each symbol and add it to the apropriate tree
    add_frequency_to_symbols(coded_vec, tree1, tree2);

    // create the actual huffman tree using the calculated frequencies
    create_huffman_tree(tree1, TREE1_NUM_SYMBOLS);
    create_huffman_tree(tree2, TREE2_NUM_SYMBOLS);

    return std::pair{tree1, tree2};
}

void Huffman_code::add_frequency_to_symbols(CodedVec& coded_vec, huffmanTree tree1,
                                            huffmanTree tree2)
{
    // calculate the frequency of each symbol
    for (uint32_t i = 0; i < coded_vec.size(); i++)
    {
        uint32_t val = coded_vec[i];

        // if val is a literal (indicated with bit = 1)
        if (val <= 255)
        {
            tree1[val].frequency++;
        }
        // else val is a window length
        else
        {
            uint32_t window_symbol = val - WINDOW_OFFSET;
            // calculate the symbol that this window lenght falls in that range
            tree1[get_symbol_from_range_for_tree_1(window_symbol)].frequency++;

            // advance the index of the vector to get the distance
            i++;

            // val is distance
            val = coded_vec[i];
            tree2[get_symbol_from_range_for_tree_2(val).symbol].frequency++;
        }
    }
}

void Huffman_code::create_huffman_tree(huffmanTree& tree, int num_symbols)
{
    // create min heap with the indexes of the tree with a custom compare function so the min heap will extract the min frequency
    auto cmp = [&tree](int left_index, int right_index)
    {
        return tree[left_index].frequency > tree[right_index].frequency;
    };

    // this queue is:
    // the symbol as its value
    // and the heap property is determined by the frequency of that symbol.
    // (note that a std::priority_queue is a wraper to a vecor and that is the reason we have it as a parameter)
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
        uint16_t node1 = min_heap_tree.top();
        min_heap_tree.pop();
        uint16_t node2 = min_heap_tree.top();
        min_heap_tree.pop();

        // merge node 1 and node 2 to a new node with the combined frequency
        // and add node 1 and node 2 as its children.
        uint32_t new_frequency = tree[node1].frequency + tree[node2].frequency;
        tree[i] = {new_frequency, node1, node2};
        i++;
    }
    // resize the tree to the last node we added to it
    tree.resize(i);
}

std::vector<uint16_t> Huffman_code::get_code_len_table(huffmanTree& tree, uint32_t table_size)
{
    // create len_table with all the symbols with len 0.
    std::vector<uint16_t> len_table(table_size, 0);

    //********* Use BFS to travers the tree to get all the code lengths *********

    // FIFO data structure that hold the node and its depth in the tree
    struct nodeDepthPair
    {
        nodeDepthPair(int node, int depth): node(node), depth(depth)
        {
        }

        int node;
        int depth;
    };
    std::deque<nodeDepthPair> deque;

    // give the root depth 0
    deque.emplace_back(tree.size() - 1, 0);

    // BFS the tree
    while (!deque.empty())
    {
        // get the top node and its depth
        int node = deque.front().node;
        int depth = deque.front().depth;
        deque.pop_back();

        // if the node is a symbol then we reached a leaf, so we can update its length
        if (node < table_size) len_table[node] = depth;

        // else we are at a intersection and we need to keep tranversing the tree
        else
        {
            // if the currunt node has children then we add them to the queu with thier depth
            if (tree[node].left != -1) deque.emplace_back(tree[node].left, depth + 1);
            if (tree[node].right != -1) deque.emplace_back(tree[node].right, depth + 1);
        }
    }
    return len_table;
}

void Huffman_code::deflate_code_length(std::vector<uint16_t>& code_length_table)
{
    std::vector<uint8_t> code_len_to_num_apearances(15, 0);
    int num_overflow = 0;

    // count how many symbols are there for each code len
    for (unsigned short i : code_length_table)
    {
        if (i <= 15 && i > 0)
        {
            code_len_to_num_apearances[i - 1] += 1;
        }
        if (i > 15)
        {
            code_len_to_num_apearances[15] += 1;
            num_overflow++;
        }
    }

    // deflating the code lenghts
    int k = 14;
    while (num_overflow > 0)
    {
        if (code_len_to_num_apearances[k - 1] == 0)
        {
            k--;
            continue;
        }

        code_len_to_num_apearances[14]--;
        code_len_to_num_apearances[k - 1]--;
        code_len_to_num_apearances[k] += 2;
        if (k < 14) k++;
        num_overflow--;
    }

    // sort the symbols by their original lenghts
    std::vector<Decode> symbol_len_table;
    for (int i = 0; i < code_length_table.size(); i++) symbol_len_table.emplace_back(i, code_length_table[i]);
    std::sort(symbol_len_table.begin(), symbol_len_table.end());

    // update the codelengths in the order of the original code lengths
    int j = 0;
    for (auto& i : symbol_len_table)
    {
        if (code_len_to_num_apearances[j] == 0)
        {
            j++;
            continue;
        }
        i.num_bits = j;
        code_len_to_num_apearances[j]--;
    }

    // updating to the new code lenghts
    for (auto& symbol_len : symbol_len_table)
    {
        code_length_table[symbol_len.symbol] = symbol_len.num_bits;
    }
}

// this method assumes the code len is not biggier than 15 bit
std::vector<HuffmanCode> Huffman_code::create_canonial_huffman_code(
    const std::vector<uint16_t>& code_len_table)
{
    // small explenation on the algorithm of biulding the canonial huffman tree

    // the index is the symbol and the table maps from the index(symbol) to the len of the code and the code(in 64 bits)
    std::vector<HuffmanCode> canonial_code(code_len_table.size(), {0, 0});

    // create a vector from symbol to len
    struct symbol_len_pair
    {
        symbol_len_pair(uint16_t symbol, uint16_t len): symbol(symbol), len(len)
        {
        }

        uint16_t symbol;
        uint16_t len;
    };
    std::vector<symbol_len_pair> symbol_to_len;
    for (size_t i = 0; i < code_len_table.size(); i++)
    {
        if (code_len_table[i] > 0) symbol_to_len.emplace_back(i, code_len_table[i]);
    }

    // sort the symbol to len by len then by symbol
    std::sort(symbol_to_len.begin(), symbol_to_len.end(), [](const auto& a, const auto& b)
    {
        return std::tie(a.len, a.symbol) < std::tie(b.len, b.symbol);
    });

    int previous_code_len = 0;
    uint16_t huffman_code = 0;
    for (const auto& symbol_to_len_pair : symbol_to_len)
    {
        if (previous_code_len < symbol_to_len_pair.len)
        {
            huffman_code = huffman_code << (symbol_to_len_pair.len - previous_code_len);
            previous_code_len = symbol_to_len_pair.len;
        }

        canonial_code[symbol_to_len_pair.symbol].len_code = symbol_to_len_pair.len;
        canonial_code[symbol_to_len_pair.symbol].huffman_code = huffman_code;
        huffman_code++;
    }

    return canonial_code;
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

    DistanceEncodeInfo info{};
    info.symbol = (((msb - 2) * 2) + 16) + b;
    info.extra_bits_len = msb - 1;

    uint32_t mask = (1 << info.extra_bits_len) - 1;
    info.extra_bits_val = val & mask;

    return info;
}


void Huffman_code::fill_table_windowLengthToCode(
    const std::vector<HuffmanCode>& canonial_code)
{
    for (int range = 0; range < windowLengthToCode.size(); range++)
    {
        int symbol = get_symbol_from_range_for_tree_1(range + 4);
        uint32_t extra_bit_val = (range + 4) - tree1_symbolToRange_table[symbol];

        windowLengthToCode[range].huffman_code = canonial_code[symbol].huffman_code;
        windowLengthToCode[range].huffman_len = canonial_code[symbol].len_code;
        windowLengthToCode[range].extra_bits_val = extra_bit_val;
        windowLengthToCode[range].extra_bits_len = std::__bit_width(extra_bit_val);
    }
}
