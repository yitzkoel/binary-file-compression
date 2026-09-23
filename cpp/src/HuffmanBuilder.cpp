//
// Created by yitzk on 9/16/2026.
//

#include "HuffmanBuilder.h"

#include <queue>
#include <tuple>


std::vector<HuffmanCode> HuffmanBuilder::get_canonial_huffman_code(const std::vector<uint16_t>& code_len_table, int max_code_len)
{
    auto code_len = code_len_table;
    assert(max_code_len <= 32 && "canoot create huffman code with max code lenght longer than 32");
    deflate_code_length(code_len, max_code_len);

    return create_canonial_huffman_code(code_len);
}

std::vector<uint8_t> HuffmanBuilder::get_code_len_table(std::vector<HuffmanCode>& huffman_code)
{
    std::vector<uint8_t> code_len_table;
    code_len_table.reserve(huffman_code.size());
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

    // edge case: there are no symbols
    if (min_heap_tree.empty())
    {
        tree.resize(0);
        return;
    }

    // edge case: there is only one symbol
    if (min_heap_tree.size() == 1)
    {
        int single_node = min_heap_tree.top();
        int dummy_node = (single_node == 0) ? 1 : 0;
        tree[dummy_node].frequency = 1;
        min_heap_tree.push(dummy_node);
    }

    // build the huffman tree
    int i = num_symbols;
    if(min_heap_tree.size() == 1) i = 1;
    while (min_heap_tree.size() > 1)
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
        min_heap_tree.push(i);
        i++;
    }
    // resize the tree to the last node we added to it
    tree.resize(i);
}

std::vector<uint16_t> HuffmanBuilder::get_code_len_table(HuffmanTree& tree, uint32_t table_size)
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
        deque.pop_front();

        // if the node is a symbol then we reached a leaf, so we can update its length
        if (node < table_size) len_table[node] = depth;

        // else we are at a intersection and we need to keep tranversing the tree
        else
        {
            // if the currunt node has children then we add them to the queu with thier depth
            if (tree[node].left != UINT16_MAX) deque.emplace_back(tree[node].left, depth + 1);
            if (tree[node].right != UINT16_MAX) deque.emplace_back(tree[node].right, depth + 1);
        }
    }
    return len_table;
}

void HuffmanBuilder::deflate_code_length(std::vector<uint16_t>& code_length_table, int max_code_len)
{
    std::vector<uint32_t> code_len_to_num_apearances(max_code_len, 0);
    int num_overflow = 0;

    // count how many symbols are there for each code len
    for (unsigned short len : code_length_table)
    {
        if (len <= max_code_len && len > 0)
        {
            code_len_to_num_apearances[len - 1] += 1;
        }
        if (len > max_code_len)
        {
            code_len_to_num_apearances[max_code_len - 1] += 1;
            num_overflow++;
        }
    }

    // deflating the code lenghts
    int k = max_code_len - 1;
    while (num_overflow > 0)
    {
        if (code_len_to_num_apearances[k - 1] == 0)
        {
            k--;
            continue;
        }

        code_len_to_num_apearances[max_code_len - 1]--;
        code_len_to_num_apearances[k - 1]--;
        code_len_to_num_apearances[k] += 2;
        if (k < max_code_len - 1) k++;
        num_overflow--;
    }

    // defining a symbol and code len pair
    struct Symbol_len_Pair
    {
        Symbol_len_Pair(uint16_t symbol, uint8_t num_bits): symbol(symbol), len(num_bits) {}

        bool operator <(const Symbol_len_Pair& other) const
        {
            if (len != other.len)
                return len < other.len;
            return symbol < other.symbol;
        }

        uint16_t symbol;
        uint8_t len;
    };

    // sort the symbols by their original lenghts
    std::vector<Symbol_len_Pair> symbol_len_table;
    for (int i = 0; i < code_length_table.size(); i++)
    {
        if (code_length_table[i] > 0)
        {
            symbol_len_table.emplace_back(i, code_length_table[i]);
        }
    }
    std::sort(symbol_len_table.begin(), symbol_len_table.end());

    // update the codelengths in the order of the original code lengths
    int j = 0;
    for (auto& symbol_len_pair : symbol_len_table)
    {
        while (j < max_code_len && code_len_to_num_apearances[j] == 0)
        {
            j++;
        }

        if (j >= max_code_len) break;

        symbol_len_pair.len = j + 1;
        code_len_to_num_apearances[j]--;
    }

    // updating to the new code lenghts
    for (const auto& symbol_len : symbol_len_table)
    {
        code_length_table[symbol_len.symbol] = symbol_len.len;
    }
}


std::vector<HuffmanCode> HuffmanBuilder::create_canonial_huffman_code(const std::vector<uint16_t>& code_len_table)
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

