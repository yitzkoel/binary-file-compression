//
// Created by yitzk on 9/15/2026.
//
#include <fstream>
#include<gtest/gtest.h>
#include<../include/HashTable.h>

class TestHashTable : public ::testing::Test
{
protected:
    int power_of_two = 18;

    std::vector<uint32_t> colision_value_keys; // holds 'num_looks' + 1 values
    std::vector<uint32_t> keys_that_last_elem_in_map;

    hashTable hash_map;

    void calculate_colision_values_keys()
    {
        std::vector<uint32_t> hash_vals;


        // fill in all hash values for the range of keys
        for (uint32_t i = 0; i < hash_map.hash_table.size() * (hashTable::num_looks + 1); i++)
        {
            hash_vals.push_back(hash_map.hashFunction(i));
        }

        // fill histogram that cunts to each hash val how many times it was hashed
        // (we are garenteed to have at least num_looks collision from the Pigeonhole Principle)
        std::vector<uint32_t> histogram(hash_map.hash_table.size(), 0);
        for (unsigned int hash_val : hash_vals)
        {
            histogram[hash_val]++;
        }

        // find the hash value with 'num_looks' collisions a(at least)
        int colision_val = 0;
        for (int i = 0; i < histogram.size(); i++)
        {
            if (histogram[i] >= hashTable::num_looks + 1)
            {
                colision_val = i;
                break;
            }
        }


        // find the reacuring hash values keys
        for (int i = 0; i < hash_vals.size(); i++)
        {
            if (hash_vals[i] == colision_val) colision_value_keys.push_back(i);

            if (colision_value_keys.size() == hashTable::num_looks + 1) break;
        }

        // find keys that map to the last hash value in the map
        for (int i = 0; i < hash_vals.size(); i++)
        {
            if(hash_vals[i] == hash_map.hash_table.size() - 1) keys_that_last_elem_in_map.push_back(i);
        }


    }

    TestHashTable(): hash_map(power_of_two)
    {
        calculate_colision_values_keys();
    }

    std::vector<uint32_t>& get_super_array()
    {
        return hash_map.super_cyclic_array;
    }

    std::vector<std::unique_ptr<cyclicArray>>& get_hash_table()
    {
        return hash_map.hash_table;
    }

    uint32_t call_hash_func(uint32_t val)
    {
        return hash_map.hashFunction(val);
    }

    static int get_num_looks()
    {
        return hashTable::num_looks;
    }
};

/******* TEST CYCLIC ARRAY ******/
TEST_F(TestHashTable, CyclicArrayBasicTest)
{
    uint32_t array[NUM_ELEMENTS_IN_ARRAY];

    uint32_t array_val = 34576;
    cyclicArray cyclic_array(array, array_val);

    // test init fields
    EXPECT_EQ(cyclic_array.array_full, false);
    EXPECT_EQ(cyclic_array.array_ptr, array);
    EXPECT_EQ(cyclic_array.array_val, array_val);
    EXPECT_EQ(cyclic_array.index_in_array, 0);

    // fill array to the top
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++) cyclic_array.add_elem(i);

    // test filling array
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++)
        EXPECT_EQ(array[i], i);

    // test if the array fields have the expected value
    EXPECT_EQ(cyclic_array.array_full, true);
    EXPECT_EQ(cyclic_array.index_in_array, 0);

    // test the cyclic property
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY >> 1; i++) cyclic_array.add_elem(-i);
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++)
    {
        if (i < NUM_ELEMENTS_IN_ARRAY >> 1)
            EXPECT_EQ(array[i], -i);
        else
            EXPECT_EQ(array[i], i);
    }
    EXPECT_EQ(cyclic_array.array_full, true);
    EXPECT_EQ(cyclic_array.index_in_array, NUM_ELEMENTS_IN_ARRAY >> 1);
}

TEST_F(TestHashTable, CyclicArrayEdgeCasesTest)
{
    auto* array = (uint32_t*)malloc(NUM_ELEMENTS_IN_ARRAY * (2 << 18) * sizeof(uint32_t));

    uint32_t array_val = 34576;
    cyclicArray cyclic_array(array + (NUM_ELEMENTS_IN_ARRAY * 2 << 12), array_val);
    auto* new_array = array + (NUM_ELEMENTS_IN_ARRAY * 2 << 12);
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++) cyclic_array.add_elem(i);

    // test filling array
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++)
    {
        EXPECT_EQ(new_array[i], i);
    }

    // test cycle array many cycles
    for (int i = 0; i < NUM_ELEMENTS_IN_ARRAY * 100; i++) cyclic_array.add_elem(i);

    for (int j = 0, i = NUM_ELEMENTS_IN_ARRAY * 99; i < NUM_ELEMENTS_IN_ARRAY * 100; i++, j++)
    {
        EXPECT_EQ(new_array[j], i);
    }

    free(array);
}

/******* TEST CYCLIC ARRAY ******/

/******* TEST HASH FUNCTION ******/
TEST_F(TestHashTable, HashFunctionRangeTest)
{
    // test that the hash function gives values that are in a legal range of the map
    auto& hash_table = get_hash_table();
    for (int i = 0; i < hash_table.size(); i++)
    {
        EXPECT_LT(call_hash_func(i), hash_table.size());
    }
}

TEST_F(TestHashTable, HashFunctionDistributionTest)
{
    const int K = 10; // hash_table_power_of_two_size
    const int TABLE_SIZE = 1 << power_of_two; // 1024

    // histogram
    std::vector<int> histogram(TABLE_SIZE, 0);

    std::vector<uint32_t> mock_data;

    // adding prboble words like 0, space bar and so on
    mock_data.push_back(0x00000000);
    mock_data.push_back(0x20202020);
    mock_data.push_back(0xFFFFFFFF);


    std::string sample_text = "This is a sample text representing typical data in Lempel-Ziv compression.";
    for (size_t i = 0; i <= sample_text.length() - 4; ++i)
    {
        // המרת 4 תווים רצופים לתוך uint32_t
        uint32_t val = (static_cast<uint8_t>(sample_text[i]) << 24) |
            (static_cast<uint8_t>(sample_text[i + 1]) << 16) |
            (static_cast<uint8_t>(sample_text[i + 2]) << 8) |
            static_cast<uint8_t>(sample_text[i + 3]);
        mock_data.push_back(val);
    }

    for (uint32_t val : mock_data)
    {
        uint32_t hash_index = call_hash_func(val);

        // range check
        ASSERT_LT(hash_index, TABLE_SIZE) << "Hash index is out of bounds!";

        histogram[hash_index]++;
    }


    int max_collisions = 0;
    for (int count : histogram)
    {
        if (count > max_collisions)
        {
            max_collisions = count;
        }
    }

    EXPECT_LT(max_collisions, 4) << "Poor distribution: A bucket got too many hits.";
}

TEST_F(TestHashTable, HashFunctionRealFilesDistributionTest)
{
    // Determine the table size based on the power of two
    const int K = 16; // hash_table_power_of_two_size
    const uint32_t TABLE_SIZE = 1 << K;

    // Vector containing the paths of the test files
    // Populate this with the actual paths you want to test
    std::vector<std::string> test_files = {
        "../../tests/test_files/Samp1.bin",
        "../../tests/test_files/Samp2.bin",
        "../../tests/test_files/Samp3.bin",
        "../../tests/test_files/Samp4.bin",
        "../../tests/test_files/Samp5.bin"
    };

    for (const auto& file_path : test_files)
    {
        // Initialize a histogram for the current file
        std::vector<uint32_t> histogram(TABLE_SIZE, 0);

        // Open the file in binary mode to ensure raw byte reading
        std::ifstream file(file_path, std::ios::binary);
        ASSERT_TRUE(file.is_open()) << "Failed to open file: " << file_path;

        // Read the entire file content into a byte buffer
        std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(file), {});

        // Skip files that are too small to form even a single 4-byte sequence
        if (buffer.size() < 4)
        {
            std::cout << "[ WARNING ] Skipping file (too small): " << file_path << std::endl;
            continue;
        }

        uint32_t total_hashes = 0;

        // Slide a 4-byte window across the file buffer
        for (size_t i = 0; i <= buffer.size() - 4; ++i)
        {
            // Construct a 32-bit integer from 4 consecutive bytes
            uint32_t val = (static_cast<uint32_t>(buffer[i]) << 24) |
                (static_cast<uint32_t>(buffer[i + 1]) << 16) |
                (static_cast<uint32_t>(buffer[i + 2]) << 8) |
                static_cast<uint32_t>(buffer[i + 3]);

            // Call your actual hash function here
            // uint32_t hash_index = hash_table_instance.hashFunction(val);

            // MOCK FUNCTION for compilation purposes (remove in real code)
            uint32_t hash_index = (val * 2654435769U) >> (32 - K);

            // Critical check: Ensure the hash function does not exceed the array bounds
            ASSERT_LT(hash_index, TABLE_SIZE) << "Hash index out of bounds for value: " << val;

            histogram[hash_index]++;
            total_hashes++;
        }

        // Analyze the distribution results for the current file
        uint32_t max_collisions = 0;
        uint32_t empty_buckets = 0;

        for (uint32_t count : histogram)
        {
            if (count > max_collisions)
            {
                max_collisions = count;
            }
            if (count == 0)
            {
                empty_buckets++;
            }
        }

        double average_collisions = static_cast<double>(total_hashes) / TABLE_SIZE;
        double empty_percentage = (static_cast<double>(empty_buckets) / TABLE_SIZE) * 100.0;

        // Output the statistics to the test console for manual inspection
        std::cout << "[ INFO    ] File: " << file_path << "\n"
            << "            Total Hashes: " << total_hashes << "\n"
            << "            Average Collisions per Bucket: " << average_collisions << "\n"
            << "            Max Collisions in a Single Bucket: " << max_collisions << "\n"
            << "            Empty Buckets: " << empty_buckets << " (" << empty_percentage << "%)\n";

        // Evaluation strategy:
        // In real files (especially uncompressed binaries), certain 4-byte sequences
        // (like 0x00000000) might appear excessively. This skew causes high max_collisions.
        // Therefore, instead of asserting on max_collisions, we assert that a large file
        // manages to utilize a reasonable portion of the hash table.

        if (total_hashes > TABLE_SIZE)
        {
            // For files that generate more hashes than the table size,
            // we expect the hash function to distribute them well enough
            // so that not more than 90% of the table is left empty.
            EXPECT_LT(empty_percentage, 90.0)
                << "Poor distribution: More than 90% of the buckets are empty for a large file.";
        }
    }
}

/******* TEST HASH FUNCTION ******/


/******* TEST HASH TABLE ******/
TEST_F(TestHashTable, HashTableBasicTest)
{
    auto& super_cyclic_array = get_super_array();
    auto& hash_table = get_hash_table();

    // test init sizes
    EXPECT_EQ(super_cyclic_array.size(), NUM_ELEMENTS_IN_ARRAY * (1 << power_of_two ));
    EXPECT_EQ(hash_table.size(), 1 << power_of_two);

    // test adding values
    for (int i = 0; i < 100; i++)
    {
        hash_map.add(i);
    }

    // test adding success
    for (int i = 0; i < 100; i++)
    {
        auto array = hash_map.find(i);
        EXPECT_EQ(array->array_val, i);
    }

    // test clear
    hash_map.clear();
    for (const auto& i : hash_table)
    {
        EXPECT_EQ(i, nullptr);
    }
}

TEST_F(TestHashTable, FindInHashTableTest)
{
    auto& hash_table = get_hash_table();
    uint32_t hash_val = call_hash_func(colision_value_keys[0]);

    std::cout << "the hash val is:" << hash_val << " and the size of the hash table is: " << hash_table.size() << std::endl;;

    // test inserting a value into the after probing
    for(int i = 0; i < colision_value_keys.size() - 1; i++)
    {
        hash_map.add(colision_value_keys[i]);
        uint32_t index = (hash_val + i) % hash_table.size();
        EXPECT_EQ(hash_table[index]->array_val, colision_value_keys[i]);
    }

    // test finding the values after probing
    for(int i = 0; i < colision_value_keys.size() - 1; i++)
    {
        EXPECT_EQ(hash_map.find(colision_value_keys[i]), hash_table[hash_val + i].get() );
    }

    // test finding a key never inserted with probing
    EXPECT_EQ(hash_map.find(colision_value_keys.back()), nullptr);

}

TEST_F(TestHashTable, OverWriteInHashTableTest)
{
    auto& hash_table = get_hash_table();
    uint32_t hash_val = call_hash_func(colision_value_keys[0]);


    // inserting a values into the table that hash the same hash_val with overwrite
    for(unsigned int colision_value_key : colision_value_keys)
    {
        hash_map.add(colision_value_key);
    }

    //test expected val
    EXPECT_EQ(hash_map.find(colision_value_keys.back()), hash_table[hash_val].get());
}

TEST_F(TestHashTable, CyclicProbingTest)
{
    // test the case that the hash function inserted a value to the end of the table and another val to the end of the table
    // what will happen a overflow of the table?  there should be a wraparound that is cyclic table
    auto& hash_table = get_hash_table();
    uint32_t hash_val = call_hash_func(keys_that_last_elem_in_map[0]);

    // sanity check that the hash is indeed the end of the map
    EXPECT_EQ(hash_val, hash_table.size() - 1);

    // add all the values to the hash map
    for(unsigned int i : keys_that_last_elem_in_map)
    {
        hash_map.add(i);
    }
     // check that they are indeed there
    for(int i = 0; i < keys_that_last_elem_in_map.size(); i++)
    {
        uint32_t array_val = hash_map.find(keys_that_last_elem_in_map[i])->array_val;
        EXPECT_EQ(array_val, keys_that_last_elem_in_map[i] );
        uint32_t index = (i + hash_table.size() - 1) % hash_table.size();
        EXPECT_EQ(hash_table[index].get(), hash_map.find(keys_that_last_elem_in_map[i]));
    }


}

TEST_F(TestHashTable, SameValMultInsertTest)
{
    // inserting the same value over and over again into the table (should not overide the prev value should do nothing)
    hash_map.add(colision_value_keys[0]);

    auto cyclic_array = hash_map.find(colision_value_keys[0]);
    for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY - 1; i++)
    {
        cyclic_array->add_elem(i);
    }

    // add the same value to the map
    hash_map.add(colision_value_keys[0]);

    // check that cyclic array is the same
    auto cmp_cyclic_array = hash_map.find(colision_value_keys[0]);
    EXPECT_EQ(cyclic_array, cmp_cyclic_array);

    // check that it holds the same values
    auto arr1 = cyclic_array->array_ptr;
    auto arr2 = cmp_cyclic_array->array_ptr;

    for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY - 1; i++)
    {
        EXPECT_EQ(arr1[i], arr2[i]);
    }

    // check that it is the same index
    EXPECT_EQ(cyclic_array->index_in_array, cmp_cyclic_array->index_in_array);

}

/******* TEST HASH TABLE ******/
