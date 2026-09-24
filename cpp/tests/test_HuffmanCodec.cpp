//
// Created by yitzk on 9/23/2026.
//

#include <BinaryIO.h>
#include <gtest/gtest.h>
#include <../include/HuffmanCodec.h>
#include <random>
#include <fstream>

/**
 * @class TestHuffmanCodec
 * @brief Test fixture for the Huffman coding engine.
 *
 * This fixture provides helper methods to generate mock LZSS-encoded vectors
 * using various statistical distributions. It also provides a unified testing
 * pipeline to compress and decompress data, ensuring lossless data recovery
 * based on the new single-block, memory-buffer API.
 */
class TestHuffmanCodec : public ::testing::Test
{
protected:
    HuffmanCodec huffman_coder;

    // Offset used to separate literals (0-255) from window lengths in the Huffman tree
    uint32_t WINDOW_OFFSET = HuffmanCodec::WINDOW_SYMBOL_OFFSET_IN_TABLE;

    // The designated End-Of-File symbol used to terminate a block
    inline static const uint16_t EOF_SYMBOL = 256;

    TestHuffmanCodec(): huffman_coder()
    {
    }

    /**
     * @brief Executes a full compression and decompression cycle on a single block.
     *
     * Writes the mock vector to a memory buffer using the new API, dumps it to a binary file
     * to simulate the Orchestrator, and decompresses it back using FileReader and BitReader.
     *
     * @param mock_lz_vector The original vector containing mock LZSS data.
     * @param test_name Base name for the generated binary file.
     */
    void run_compression_cycle(CodedVec& mock_lz_vector, const std::string& test_name)
    {
        std::string coded_vec_file_path = test_name + ".bin";

        std::cout << ". Running compression phase..." << std::endl;
        huffman_coder.clear();

        // Allocate an 8MB buffer simulating the thread-pool buffer provision
        std::vector<uint8_t> out_buffer(8 * 1024 * 1024, 0);

        // Step 1: Compress the mock vector and GET THE EXACT SIZE
        uint64_t compressed_size = huffman_coder.compress(out_buffer.data(), mock_lz_vector);

        // Simulate Orchestrator writing ONLY the valid compressed bytes to disk
        {
            std::ofstream out_file(coded_vec_file_path, std::ios::binary);
            out_file.write(reinterpret_cast<const char*>(out_buffer.data()), compressed_size);
            out_file.close();
        }

        std::cout << ". Running decompression phase..." << std::endl;
        huffman_coder.clear();

        // Step 2: Initialize readers simulating the reading pipeline
        BinaryIO::FileReader file_reader(coded_vec_file_path);
        file_reader.slide_window();

        BitReader bit_reader(file_reader.get_buffer(), file_reader.get_num_bytes_read());

        // Injecting the dependency configuration:
        // The orchestrator defines the safe boundary for reading (8 bytes margin for 64-bit window).
        bit_reader.set_safe_end(8);

        // Decompress the binary file back into memory
        auto res_vec = huffman_coder.decompress(file_reader, bit_reader);

        // Step 3: Verify the integrity of the data within the block
        ASSERT_EQ(res_vec.size(), mock_lz_vector.size()) << "Vector size mismatch at block level!";
        for (size_t j = 0; j < res_vec.size(); j++)
        {
            EXPECT_EQ(res_vec[j], mock_lz_vector[j]) << "Data mismatch at element index " << j;
        }
    }

    // ==========================================================
    // Random Data Generators
    // These functions generate data based on different statistical
    // distributions to stress-test the Huffman tree builder.
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

TEST_F(TestHuffmanCodec, BasicTest)
{
    std::cout << "[INFO] Running Basic Test: Validating standard literal, length, and distance encoding.\n";

    // Setup a basic, hardcoded LZSS formatted vector
    CodedVec mock_vec = {
        'a', 'a', 'b', 5 + WINDOW_OFFSET, 3, 'c', 'd', 4 + WINDOW_OFFSET, 2, 'd', 'a', 'b', 'c', 'e',
        4 + WINDOW_OFFSET, 4, 'd', 6 + WINDOW_OFFSET, 21, 'f', 'g', EOF_SYMBOL
    };

    run_compression_cycle(mock_vec, "HuffmanBasicTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestEmptyVec)
{
    std::cout << "[INFO] Running Empty Vector Test: Validating edge case with zero data (EOF only).\n";

    // Setup a vector containing only the termination symbol
    CodedVec mock_vec = { EOF_SYMBOL };

    run_compression_cycle(mock_vec, "HuffmanEmptyTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Extreme Ratio & Edge Case Tests
// ==========================================================

TEST_F(TestHuffmanCodec, TestHomogeneousData)
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

    run_compression_cycle(mock_vec, "HomogeneousTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Statistical Distribution Tests
// Note: To preserve LZSS syntax integrity, random values are
// clamped to <= 255. This forces them to be processed as literals,
// simulating completely uncompressible data blocks.
// ==========================================================

TEST_F(TestHuffmanCodec, TestRandomUniformDistribution)
{
    std::cout << "[INFO] Running Uniform Distribution Test: Simulating uncompressible data (forces a perfectly balanced Huffman tree).\n";

    // Values strictly between 0 and 255 (literals only)
    CodedVec mock_vec = generate_random_uniform_vector(10000, 0, 255);
    mock_vec.push_back(EOF_SYMBOL);

    run_compression_cycle(mock_vec, "UniformTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestRandomPoissonDistribution)
{
    std::cout << "[INFO] Running Poisson Distribution Test: Simulating natural language text frequencies.\n";

    CodedVec mock_vec = generate_random_poisson_vector(10000, 50.0);

    // Clamp values to <= 255 to maintain LZSS literal syntax
    for (auto& val : mock_vec) { if (val > 255) val = 255; }
    mock_vec.push_back(EOF_SYMBOL);

    run_compression_cycle(mock_vec, "PoissonTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestRandomBinomialDistribution)
{
    std::cout << "[INFO] Running Binomial Distribution Test: Simulating bell-curve data frequencies.\n";

    // Binomial with 255 trials peaks naturally within the literal range
    CodedVec mock_vec = generate_random_binomial_vector(10000, 255, 0.5);

    // Safety clamp to guarantee format compliance
    for (auto& val : mock_vec) { if (val > 255) val = 255; }
    mock_vec.push_back(EOF_SYMBOL);

    run_compression_cycle(mock_vec, "BinomialTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestRandomGeometricDistribution)
{
    std::cout << "[INFO] Running Geometric Distribution Test: Simulating heavy long-tail frequencies (stresses deep Huffman tree limits).\n";

    CodedVec mock_vec = generate_random_geometric_vector(10000, 0.1);

    // Geometric distributions can generate unbounded upper values; clamping is required
    for (auto& val : mock_vec) { if (val > 255) val = 255; }
    mock_vec.push_back(EOF_SYMBOL);

    run_compression_cycle(mock_vec, "GeometricTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

// ==========================================================
// Robustness and Stress Tests
// ==========================================================

TEST_F(TestHuffmanCodec, TestDeterministicLZSS_Debug)
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

    run_compression_cycle(mock_vec, "DebugLZSSTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestRandomValidLZSS)
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

    run_compression_cycle(mock_vec, "RandomValidLZSSTest");

    std::cout << "*************** Test Finished ***********\n\n";
}

TEST_F(TestHuffmanCodec, TestBounderySymbols)
{
    std::cout << "[INFO] Running Boundary Symbols Test: Ensuring safe processing of maximum supported lengths and distances.\n";

    // Constructs a sequence utilizing the absolute maximum literal, window length, and distance bounds
    CodedVec mock_vec = {
        255, 2068, 1 << 11, 255, 2068, 1 << 11, 255, 2068, 1 << 11, 255, 2068, 1 << 11, EOF_SYMBOL
    };

    run_compression_cycle(mock_vec, "HuffmanBounderySymbolTest");

    std::cout << "*************** Test Finished ***********\n\n";
}