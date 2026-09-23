//
// Created by yitzk on 9/18/2026.
//

#include <gtest/gtest.h>
#include <bitset>
#include <random>
#include <../include/HuffmanBuilder.h>

class TestHuffmanBuilder : public ::testing::Test
{
public:
    TestHuffmanBuilder() = default;
protected:
};

void printLowBits(uint32_t num, int k)
{
    if (k <= 0) return;
    if (k > 32) k = 32;

    // Simple loop from the (k-1)-th bit down to bit 0
    for (int i = k - 1; i >= 0; --i)
    {
        std::cout << ((num >> i) & 1);
    }
}

std::vector<uint32_t> generateFibonacci_sequence_vec(size_t num_symbols)
{
    if (num_symbols == 0) return {};

    std::vector<uint32_t> fib(num_symbols);
    fib[0] = 1;
    if (num_symbols > 1) fib[1] = 1;
    for (size_t i = 2; i < num_symbols; ++i)
    {
        fib[i] = fib[i - 1] + fib[i - 2];
    }

    std::vector<uint32_t> raw_data;
    for (size_t sym = 0; sym < num_symbols; ++sym)
    {
        uint8_t symbol_val = static_cast<uint8_t>(sym);
        raw_data.insert(raw_data.end(), fib[sym], symbol_val);
    }

    return raw_data;
}

uint32_t mapper(uint32_t val) { return val; }


std::vector<uint32_t> generate_random_uniform_vector(int k, int min_val, int max_val) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> distrib(min_val, max_val);

    std::vector<uint32_t> vec;
    vec.reserve(k);

    for (int i = 0; i < k; ++i) {
        vec.push_back(distrib(gen));
    }

    return vec;
}

std::vector<uint32_t> generate_random_binomial_vector(int k, int trials, double p) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::binomial_distribution<int> binom_dist(trials, p);

    std::vector<uint32_t> vec;
    vec.reserve(k);

    for (int i = 0; i < k; ++i) {
        vec.push_back(binom_dist(gen));
    }

    return vec;
}

std::vector<uint32_t> generate_random_poisson_vector(int k, double mean) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::poisson_distribution<int> poisson_dist(mean);

    std::vector<uint32_t> vec;
    vec.reserve(k);

    for (int i = 0; i < k; ++i) {
        vec.push_back(poisson_dist(gen));
    }

    return vec;
}

std::vector<uint32_t> generate_random_geometric_vector(int k, double p) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::geometric_distribution<int> geom_dist(p);

    std::vector<uint32_t> vec;
    vec.reserve(k);

    for (int i = 0; i < k; ++i) {
        vec.push_back(geom_dist(gen));
    }

    return vec;
}

// Verifies that the generated Huffman codes strictly follow DEFLATE rules and mathematical properties
void verify_huffman_properties(const std::vector<HuffmanCode>& codes, int max_len = 15) {
    uint32_t kraft_sum = 0;
    uint32_t max_kraft_capacity = 1 << max_len; // 2^max_len (32768 for 15 bits)

    for (int i = 0; i < codes.size(); i++) {
        if (codes[i].len_code == 0) continue; // Skip symbols that do not appear in the data

        // 1. Check maximum length constraint
        EXPECT_LE(codes[i].len_code, max_len) << "Symbol " << i << " exceeded max length!";

        // 2. Add to Kraft sum calculation
        kraft_sum += (1 << (max_len - codes[i].len_code));

        // 3. Prefix collision check (ensure no short code is a prefix of a longer code)
        for (int j = 0; j < codes.size(); j++) {
            if (i == j || codes[j].len_code == 0) continue;

            // If code 'i' is shorter or equal to code 'j', ensure 'i' is not a prefix of 'j'
            if (codes[i].len_code <= codes[j].len_code) {
                // Shift code 'j' right to compare only the top bits
                uint32_t shifted = codes[j].huffman_code >> (codes[j].len_code - codes[i].len_code);
                EXPECT_NE(codes[i].huffman_code, shifted)
                    << "Prefix collision! Symbol " << i << " is a prefix of Symbol " << j;
            }
        }
    }
    // Verify the final Kraft inequality sum (must not exceed tree capacity)
    EXPECT_LE(kraft_sum, max_kraft_capacity) << "Kraft inequality violated! Tree is over-full.";
}


// ---------------------------------------------------------
// HARDCODED EDGE-CASE TESTS
// ---------------------------------------------------------

TEST_F(TestHuffmanBuilder, Test_get_canonial_huffman_code_Basic_Test)
{
    std::cout << "[INFO] Basic flat distribution - Test_get_canonial_huffman_code_Basic_Test\n";

    // Test a basic array where all 4 symbols appear exactly once.
    // Expectation: A perfectly balanced tree where each symbol receives a 2-bit code.
    std::vector<uint32_t> code_vec = {0, 1, 2, 3};
    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(code_vec, mapper, 4, 15);

    for (int i = 0; i < code.size(); i++)
    {
        EXPECT_EQ(code[i].len_code, 2);
        std::cout << "the code of symbol " << i << " is:";
        printLowBits(code[i].huffman_code, code[i].len_code);
        std::cout << std::endl;
    }
}

TEST_F(TestHuffmanBuilder, Test_get_canonial_huffman_code_Advance_Test1)
{
    std::cout << "[INFO] Powers of two - Test_get_canonial_huffman_code_Advance_Test1\n";

    // Test frequencies based on powers of two (8, 4, 2, 1).
    // Expectation: The Huffman tree inherently matches this ideal distribution,
    // resulting in strictly increasing lengths of 1, 2, 3, and 3 bits.
    std::vector<uint32_t> code_vec = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3};

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(code_vec, mapper, 4, 15);
    std::vector<uint32_t> expected_code_len = {1, 2, 3, 3};

    for (int i = 0; i < code.size(); i++)
    {
        EXPECT_EQ(code[i].len_code, expected_code_len[i]);
        std::cout << "the code of symbol " << i << " is:";
        printLowBits(code[i].huffman_code, code[i].len_code);
        std::cout << std::endl;
    }
}

TEST_F(TestHuffmanBuilder, Test_get_canonial_huffman_code_Advance_Test2)
{
    std::cout << "[INFO] Fibonacci length limit - Test_get_canonial_huffman_code_Advance_Test2\n";

    // Test the Fibonacci sequence which inherently produces extremely deep trees.
    // Expectation: The tree depth will exceed the 15-bit DEFLATE limit.
    // The algorithm must correctly activate the Kraft adjustment to limit max length to 15.
    std::vector<uint32_t> code_vec = generateFibonacci_sequence_vec(18);

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(code_vec, mapper, 18, 15);
    std::vector<uint32_t> expected_code_len = {
        15, 15, 15, 15, 15, // symbols 0, 1, 2, 3, 4
        14,                 // symbol 5
        12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 // symbols 6 through 17
    };

    for (int i = 0; i < code.size(); i++)
    {
        EXPECT_EQ(code[i].len_code, expected_code_len[i]);
        std::cout << "the code of symbol " << i << " is:";
        printLowBits(code[i].huffman_code, code[i].len_code);
        std::cout << std::endl;
    }
}

TEST_F(TestHuffmanBuilder, Test_get_canonial_huffman_code_Advance_Test3)
{
    std::cout << "[INFO] Single symbol edge-case - Test_get_canonial_huffman_code_Advance_Test3\n";

    // Test the extreme edge case where only a single valid symbol exists in the data.
    // Expectation: The algorithm must inject a dummy node to form a valid binary tree.
    // The active symbol and the dummy should both receive a length of 1, while unused symbols get 0.
    std::vector<uint32_t> code_vec = {0};

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(code_vec, mapper, 8, 15);
    std::vector<uint32_t> expected_code_len = {1, 1, 0, 0, 0, 0, 0, 0};

    for (int i = 0; i < code.size(); i++)
    {
        EXPECT_EQ(code[i].len_code, expected_code_len[i]);
        if (code[i].len_code > 0)
        {
            std::cout << "the code of symbol " << i << " is:";
            printLowBits(code[i].huffman_code, code[i].len_code);
            std::cout << std::endl;
        }
    }
}

TEST_F(TestHuffmanBuilder, Test_get_canonial_huffman_code_Advance_Test4)
{
    std::cout << "[INFO] Perfectly balanced tree - Test_get_canonial_huffman_code_Advance_Test4\n";

    // Test a flat distribution of exactly 32 distinct symbols (power of 2).
    // Expectation: The algorithm should produce a perfectly balanced tree
    // where every single symbol is assigned a code length of exactly 5 bits (2^5 = 32).
    std::vector<uint32_t> code_vec;
    code_vec.reserve(32);
    for (int i = 0; i < 32; i++) code_vec.emplace_back(i);

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(code_vec, mapper, 32, 15);

    for (int i = 0; i < code.size(); i++)
    {
        EXPECT_EQ(code[i].len_code, 5);
        std::cout << "the code of symbol " << i << " is:";
        printLowBits(code[i].huffman_code, code[i].len_code);
        std::cout << std::endl;
    }
}


// ---------------------------------------------------------
// RANDOM PROPERTY-BASED TESTS
// ---------------------------------------------------------

TEST_F(TestHuffmanBuilder, Test_Random_Uniform_Distribution)
{
    std::cout << "[INFO] Random uniform distribution - Test_Random_Uniform_Distribution\n";

    // Generate 10,000 elements uniformly distributed between 0 and 255.
    // Simulates incompressible data (like encrypted or already compressed files).
    // Expectation: Most symbols will center around 8 bits, and the tree must pass all mathematical checks.
    std::vector<uint32_t> data_vec = generate_random_uniform_vector(10000, 0, 255);

    auto max_it = std::max_element(data_vec.begin(), data_vec.end());
    int num_symbols = (max_it != data_vec.end()) ? (*max_it + 1) : 0;

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(data_vec, mapper, num_symbols, 15);

    for (int i = 0; i < code.size(); i++)
    {
        if (code[i].len_code > 0)
        {
            std::cout << "the code of symbol " << i << " is:";
            printLowBits(code[i].huffman_code, code[i].len_code);
            std::cout << std::endl;
        }
    }

    verify_huffman_properties(code, 15);
}

TEST_F(TestHuffmanBuilder, Test_Random_Binomial_Distribution)
{
    std::cout << "[INFO] Random binomial distribution - Test_Random_Binomial_Distribution\n";

    // Generate 10,000 elements using a binomial bell curve (centered around 127).
    // Simulates data with a strong center tendency but smooth drop-offs.
    // Expectation: Valid tree generation passing all structural rules.
    std::vector<uint32_t> data_vec = generate_random_binomial_vector(10000, 255, 0.5);

    auto max_it = std::max_element(data_vec.begin(), data_vec.end());
    int num_symbols = (max_it != data_vec.end()) ? (*max_it + 1) : 0;

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(data_vec, mapper, num_symbols, 15);

    for (int i = 0; i < code.size(); i++)
    {
        if (code[i].len_code > 0)
        {
            std::cout << "the code of symbol " << i << " is:";
            printLowBits(code[i].huffman_code, code[i].len_code);
            std::cout << std::endl;
        }
    }

    verify_huffman_properties(code, 15);
}

TEST_F(TestHuffmanBuilder, Test_Random_Poisson_Distribution)
{
    std::cout << "[INFO] Random Poisson distribution - Test_Random_Poisson_Distribution\n";

    // Generate 10,000 elements using a Poisson distribution (mean = 50.0).
    // Highly representative of real-world text or filtered image data.
    // Expectation: A tree with short codes around the mean, long tails, and passing all rules.
    std::vector<uint32_t> data_vec = generate_random_poisson_vector(10000, 50.0);

    auto max_it = std::max_element(data_vec.begin(), data_vec.end());
    int num_symbols = (max_it != data_vec.end()) ? (*max_it + 1) : 0;

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(data_vec, mapper, num_symbols, 15);

    for (int i = 0; i < code.size(); i++)
    {
        if (code[i].len_code > 0)
        {
            std::cout << "the code of symbol " << i << " is:";
            printLowBits(code[i].huffman_code, code[i].len_code);
            std::cout << std::endl;
        }
    }

    verify_huffman_properties(code, 15);
}

TEST_F(TestHuffmanBuilder, Test_Random_Geometric_Distribution)
{
    std::cout << "[INFO] Random geometric distribution - Test_Random_Geometric_Distribution\n";

    // Generate 10,000 elements using a Geometric distribution.
    // Creates extreme long-tail redundancy, aggressively testing tree depth handling.
    // Expectation: The algorithm must enforce the 15-bit limit while keeping a valid Canonical tree.
    std::vector<uint32_t> data_vec = generate_random_geometric_vector(10000, 0.1);

    auto max_it = std::max_element(data_vec.begin(), data_vec.end());
    int num_symbols = (max_it != data_vec.end()) ? (*max_it + 1) : 0;

    std::vector<HuffmanCode> code = HuffmanBuilder::get_canonial_huffman_code(data_vec, mapper, num_symbols, 15);

    for (int i = 0; i < code.size(); i++)
    {
        if (code[i].len_code > 0)
        {
            std::cout << "the code of symbol " << i << " is:";
            printLowBits(code[i].huffman_code, code[i].len_code);
            std::cout << std::endl;
        }
    }

    verify_huffman_properties(code, 15);
}