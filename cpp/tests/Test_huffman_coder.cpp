//
// Created by yitzk on 9/23/2026.
//

#include <gtest/gtest.h>
#include <../include/huffman_code.h>
#include <random>

/**
 * @class TestHuffmanCoder
 * @brief Test fixture for the Huffman coding engine.
 *
 * This fixture provides helper methods to generate mock LZSS-encoded vectors
 * using various statistical distributions. It also provides a unified testing
 * pipeline to compress and decompress data, ensuring lossless data recovery
 * and verifying buffer boundary behaviors.
 */
class TestHuffmanCoder : public ::testing::Test
{
protected:
    Huffman_code huffman_coder;

    // Offset used to separate literals (0-255) from window lengths in the Huffman tree
    uint32_t WINDOW_OFFSET = Huffman_code::WINDOW_SYMBOL_OFFSET_IN_TABLE;

    // The designated End-Of-File symbol used to terminate a block
    inline static const uint16_t EOF_SYMBOL = 256;

    TestHuffmanCoder(): huffman_coder()
    {
    }

    /**
     * @brief Executes a full compression and decompression cycle.
     *
     * Writes the mock vectors to a binary file, decompresses them back into memory,
     * and performs a deep equality check against the original vectors.
     *
     * @param mock_lz_vectors The original vectors containing mock LZSS data.
     * @param original_file_size The theoretical uncompressed file size (used for headers).
     * @param test_name Base name for the generated binary file.
     */
    void run_compression_cycle(const std::vector<CodedVec>& mock_lz_vectors, uint64_t original_file_size,
                               const std::string& test_name)
    {
        std::string coded_vec_file_path = test_name + ".bin";

        std::cout << ". Running compression phase..." << std::endl;
        // Step 1: Compress the mock vectors into a binary file
        huffman_coder.compress(coded_vec_file_path, mock_lz_vectors, original_file_size);

        std::cout << ". Running decompression phase..." << std::endl;
        // Step 2: Decompress the binary file back into memory
        auto res_vecs = huffman_coder.decompress(coded_vec_file_path);

        // Step 3: Verify that the exact number of blocks was recovered
        ASSERT_EQ(res_vecs.size(), mock_lz_vectors.size()) << "Number of recovered blocks does not match the input!";

        // Step 4: Verify the integrity of the data within each block
        for (size_t i = 0; i < res_vecs.size(); i++)
        {
            ASSERT_EQ(res_vecs[i].size(), mock_lz_vectors[i].size()) << "Vector size mismatch at block index " << i;
            for (size_t j = 0; j < res_vecs[i].size(); j++)
            {
                EXPECT_EQ(res_vecs[i][j], mock_lz_vectors[i][j]) << "Data mismatch at block " << i << ", element index " << j;
            }
        }
    }

    // ==========================================================
    // Random Data Generators
    // These functions generate data based on different statistical
    // distributions to stress-test the Huffman tree builder, forcing
    // it to handle perfectly balanced trees, highly skewed trees, etc.
    // ==========================================================

    std::vector<uint32_t> generate_random_uniform_vector(int k, int min_val, int max_val)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> distrib(min_val, max_val);

        std::vector<uint32_t> vec;
        vec.reserve(k);

        for (int i = 0; i < k; ++i)
        {
            vec.push_back(distrib(gen));
        }

        return vec;
    }

    std::vector<uint32_t> generate_random_binomial_vector(int k, int trials, double p)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::binomial_distribution<int> binom_dist(trials, p);

        std::vector<uint32_t> vec;
        vec.reserve(k);

        for (int i = 0; i < k; ++i)
        {
            vec.push_back(binom_dist(gen));
        }

        return vec;
    }

    std::vector<uint32_t> generate_random_poisson_vector(int k, double mean)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::poisson_distribution<int> poisson_dist(mean);

        std::vector<uint32_t> vec;
        vec.reserve(k);

        for (int i = 0; i < k; ++i)
        {
            vec.push_back(poisson_dist(gen));
        }

        return vec;
    }

    std::vector<uint32_t> generate_random_geometric_vector(int k, double p)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::geometric_distribution<int> geom_dist(p);

        std::vector<uint32_t> vec;
        vec.reserve(k);

        for (int i = 0; i < k; ++i)
        {
            vec.push_back(geom_dist(gen));
        }

        return vec;
    }
};

// ==========================================================
// Core Functionality Tests
// ==========================================================

TEST_F(TestHuffmanCoder, BasicTest)
{
    std::cout << "[INFO] Running Basic Test: Validating standard literal, length, and distance encoding.\n";

    // Setup a basic, hardcoded LZSS formatted vector
    std::vector<CodedVec> mock_lz_vectors = {
        {
            'a', 'a', 'b', 5 + WINDOW_OFFSET, 3, 'c', 'd', 4 + WINDOW_OFFSET, 2, 'd', 'a', 'b', 'c', 'e',
            4 + WINDOW_OFFSET, 4, 'd', 6 + WINDOW_OFFSET, 21, 'f', 'g', EOF_SYMBOL
        }
    };

    uint64_t original_file_size = 32;
    run_compression_cycle(mock_lz_vectors, original_file_size, "HuffmanBasicTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestEmptyVec)
{
    std::cout << "[INFO] Running Empty Vector Test: Validating edge case with zero data (EOF only).\n";

    // Setup a vector containing only the termination symbol
    std::vector<CodedVec> mock_lz_vectors = {
        {
            EOF_SYMBOL
        }
    };

    uint64_t original_file_size = 0;
    run_compression_cycle(mock_lz_vectors, original_file_size, "HuffmanEmptyTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestMultVecs)
{
    std::cout << "[INFO] Running Multi-Block Test: Validating compression and parsing of multiple independent blocks.\n";

    // Setup a standard block structure
    CodedVec mock_vec = {
        'a', 'a', 'b', 5 + WINDOW_OFFSET, 3, 'c', 'd', 4 + WINDOW_OFFSET, 2, 'd', 'a', 'b', 'c', 'e',
        4 + WINDOW_OFFSET, 4, 'd', 6 + WINDOW_OFFSET, 21, 'f', 'g', EOF_SYMBOL
    };

    // Duplicate the block to simulate a file split across multiple LZSS chunks
    std::vector<CodedVec> mock_lz_vectors;
    int num_vecs = 4;
    mock_lz_vectors.reserve(num_vecs);
    for (int i = 0; i < num_vecs; i++)
    {
        mock_lz_vectors.push_back(mock_vec);
    }

    uint64_t original_file_size = 32 * num_vecs;
    run_compression_cycle(mock_lz_vectors, original_file_size, "HuffmanMultBlocksTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Extreme Ratio & Edge Case Tests
// ==========================================================

TEST_F(TestHuffmanCoder, TestHomogeneousData)
{
    std::cout << "[INFO] Running Homogeneous Data Test: Forcing maximum compression ratio (highly skewed Huffman tree).\n";

    CodedVec mock_vec;
    mock_vec.push_back('A'); // Provide a single literal to seed the decoder

    // Simulate an LZSS engine finding 10,000 continuous matches of maximum length and minimum distance
    for (int i = 0; i < 10000; i++)
    {
        mock_vec.push_back(2068 + WINDOW_OFFSET); // Maximum supported window length
        mock_vec.push_back(1); // Minimum distance
    }
    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> mock_lz_vectors = {mock_vec};

    // Theoretical size: 1 initial byte + (10,000 matches * 2,068 bytes)
    uint64_t original_file_size = 1 + (10000 * 2068);
    run_compression_cycle(mock_lz_vectors, original_file_size, "HomogeneousTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Statistical Distribution Tests
// Note: To preserve LZSS syntax integrity, random values are
// clamped to <= 255. This forces them to be processed as literals,
// simulating completely uncompressible data blocks.
// ==========================================================

TEST_F(TestHuffmanCoder, TestRandomUniformDistribution)
{
    std::cout << "[INFO] Running Uniform Distribution Test: Simulating uncompressible data (forces a perfectly balanced Huffman tree).\n";

    // Values strictly between 0 and 255 (literals only)
    CodedVec mock_vec = generate_random_uniform_vector(10000, 0, 255);
    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> mock_lz_vectors = {mock_vec};
    uint64_t original_file_size = 10000;
    run_compression_cycle(mock_lz_vectors, original_file_size, "UniformTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestRandomPoissonDistribution)
{
    std::cout << "[INFO] Running Poisson Distribution Test: Simulating natural language text frequencies.\n";

    CodedVec mock_vec = generate_random_poisson_vector(10000, 50.0);

    // Clamp values to <= 255 to maintain LZSS literal syntax
    for (auto& val : mock_vec) { if (val > 255) val = 255; }

    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> mock_lz_vectors = {mock_vec};
    uint64_t original_file_size = 10000;
    run_compression_cycle(mock_lz_vectors, original_file_size, "PoissonTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestRandomBinomialDistribution)
{
    std::cout << "[INFO] Running Binomial Distribution Test: Simulating bell-curve data frequencies.\n";

    // Binomial with 255 trials peaks naturally within the literal range
    CodedVec mock_vec = generate_random_binomial_vector(10000, 255, 0.5);

    // Safety clamp to guarantee format compliance
    for (auto& val : mock_vec) { if (val > 255) val = 255; }

    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> mock_lz_vectors = {mock_vec};
    uint64_t original_file_size = 10000;
    run_compression_cycle(mock_lz_vectors, original_file_size, "BinomialTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestRandomGeometricDistribution)
{
    std::cout << "[INFO] Running Geometric Distribution Test: Simulating heavy long-tail frequencies (stresses deep Huffman tree limits).\n";

    CodedVec mock_vec = generate_random_geometric_vector(10000, 0.1);

    // Geometric distributions can generate unbounded upper values; clamping is required
    for (auto& val : mock_vec) { if (val > 255) val = 255; }

    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> mock_lz_vectors = {mock_vec};
    uint64_t original_file_size = 10000;
    run_compression_cycle(mock_lz_vectors, original_file_size, "GeometricTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Robustness and Stress Tests
// ==========================================================

TEST_F(TestHuffmanCoder, TestDeterministicLZSS_Debug)
{
    std::cout << "[INFO] Running Deterministic LZSS Test: Fixed seed for reproducible debugging of extra-bits encoding logic.\n";

    std::mt19937 gen(1337); // Fixed seed ensures the exact same vector is generated every time

    std::uniform_int_distribution<int> coin_flip(0, 1);
    std::uniform_int_distribution<uint32_t> literal_dist(0, 255);
    std::uniform_int_distribution<uint32_t> length_dist(4, 2068);
    std::uniform_int_distribution<uint32_t> distance_dist(1, 32768);

    CodedVec mock_vec;
    mock_vec.reserve(25000);

    // Simulate 10,000 valid LZSS state transitions (either a literal or a length+distance pair)
    int iterations = 10000;
    for (int i = 0; i < iterations; ++i)
    {
        if (coin_flip(gen) == 0)
        {
            mock_vec.push_back(literal_dist(gen));
        }
        else
        {
            mock_vec.push_back(length_dist(gen) + WINDOW_OFFSET);
            mock_vec.push_back(distance_dist(gen));
        }
    }
    mock_vec.push_back(EOF_SYMBOL);

    std::cout << "[INFO] Generated test vector size: " << mock_vec.size() << " elements." << std::endl;

    std::vector<CodedVec> blocks = {mock_vec};
    uint64_t original_file_size = 50000;

    run_compression_cycle(blocks, original_file_size, "DebugLZSSTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestRandomValidLZSS)
{
    std::cout << "[INFO] Running Randomized LZSS Stress Test: Testing random valid literal/match combinations.\n";

    std::random_device rd;
    std::mt19937 gen(rd());

    // 50% probability for a literal, 50% probability for a match (length + distance pair)
    std::uniform_int_distribution<int> coin_flip(0, 1);

    std::uniform_int_distribution<uint32_t> literal_dist(0, 255);
    std::uniform_int_distribution<uint32_t> length_dist(4, 2068);

    // Distances up to 32,768 force the engine to utilize its maximum extra-bits logic
    std::uniform_int_distribution<uint32_t> distance_dist(1, 32768);

    CodedVec mock_vec;
    mock_vec.reserve(20000);

    for (int i = 0; i < 10000; ++i)
    {
        if (coin_flip(gen) == 0)
        {
            mock_vec.push_back(literal_dist(gen));
        }
        else
        {
            mock_vec.push_back(length_dist(gen) + WINDOW_OFFSET);
            mock_vec.push_back(distance_dist(gen));
        }
    }

    mock_vec.push_back(EOF_SYMBOL);

    std::vector<CodedVec> blocks = {mock_vec};
    uint64_t original_file_size = 50000;

    run_compression_cycle(blocks, original_file_size, "RandomValidLZSSTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestBounderySymbols)
{
    std::cout << "[INFO] Running Boundary Symbols Test: Ensuring safe processing of maximum supported lengths and distances.\n";

    // Constructs a sequence utilizing the absolute maximum literal, window length, and distance bounds
    CodedVec mock_vec = {
        255, 2068, 1 << 11, 255, 2068, 1 << 11, 255, 2068, 1 << 11, 255, 2068, 1 << 11, EOF_SYMBOL
    };

    std::vector<CodedVec> mock_lz_vectors;
    int num_vecs = 4;
    mock_lz_vectors.reserve(num_vecs);
    for (int i = 0; i < num_vecs; i++)
    {
        mock_lz_vectors.push_back(mock_vec);
    }

    uint64_t original_file_size = 32 * num_vecs;
    run_compression_cycle(mock_lz_vectors, original_file_size, "HuffmanBounderySymbolTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCoder, TestBufferCycleOver4MB)
{
    std::cout << "[INFO] Running 4MB Buffer Cycle Test: Forcing the IO reader to cycle buffers mid-decompression.\n";

    std::mt19937 gen(42);
    std::uniform_int_distribution<uint32_t> literal_dist(0, 255);

    std::vector<CodedVec> blocks;

    // Generate 5 blocks containing 1,000,000 random elements each.
    // Due to the uniform distribution, this will output >4MB of compressed Huffman data,
    // explicitly triggering the internal buffer refill logic (`read_new_data_into_buffer`).
    for (int b = 0; b < 5; ++b)
    {
        CodedVec mock_vec;
        mock_vec.reserve(1000000 + 1);

        for (int i = 0; i < 1000000; ++i)
        {
            mock_vec.push_back(literal_dist(gen));
        }
        mock_vec.push_back(EOF_SYMBOL);
        blocks.push_back(mock_vec);
    }

    uint64_t original_file_size = 5000000;

    // The decompression phase must successfully traverse the buffer cycle without throwing memory bounds errors
    EXPECT_NO_THROW({
        run_compression_cycle(blocks, original_file_size, "BufferCycleTest");
    });

    std::cout << "*************** Test Finished ***********\n\n";
}