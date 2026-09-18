//
// Created by yitzk on 9/16/2026.
//

#include "HuffmanBuilder.h"

#include <queue>
#include <tuple>

HuffmanBuilder::HuffmanBuilder(int max_code_len)
{
    assert(max_code_len <= 32 && "canoot create huffman code with max code lenght longer than 32");
}

std::vector<HuffmanCode> HuffmanBuilder::get_canonial_huffman_code(const std::vector<uint8_t>& code_len_table, int max_code_len)
{
    auto code_len = code_len_table;
    assert(max_code_len <= 32 && "canoot create huffman code with max code lenght longer than 32");
    deflate_code_length(code_len, max_code_len);

    return create_canonial_huffman_code(code_len);
}

std::vector<uint8_t> HuffmanBuilder::get_code_len_table(std::vector<HuffmanCode>& huffman_code)
{
    std::vector<uint8_t> code_len_table;
    for(auto& code : huffman_code)
    {
        code_len_table.push_back(code.len_code);
    }
    return code_len_table;
}

void HuffmanBuilder::create_huffman_tree(HuffmanTree& tree, int num_symbols)
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

std::vector<uint8_t> HuffmanBuilder::get_code_len_table(HuffmanTree& tree, uint32_t table_size)
{
    // create len_table with all the symbols with len 0.
    std::vector<uint8_t> len_table(table_size, 0);

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

void HuffmanBuilder::deflate_code_length(std::vector<uint8_t>& code_length_table,int max_code_len)
{
    std::vector<uint8_t> code_len_to_num_apearances(max_code_len, 0);
    int num_overflow = 0;

    // count how many symbols are there for each code len
    for (unsigned short i : code_length_table)
    {
        if (i <= max_code_len && i > 0)
        {
            code_len_to_num_apearances[i - 1] += 1;
        }
        if (i > max_code_len)
        {
            code_len_to_num_apearances[max_code_len - 1] += 1;
            num_overflow++;
        }
    }
}


std::vector<HuffmanCode> HuffmanBuilder::create_canonial_huffman_code(const std::vector<uint8_t>& code_len_table)
{
        // TODO small explenation on the algorithm of building the canonial huffman tree

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
        uint32_t huffman_code = 0;
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

