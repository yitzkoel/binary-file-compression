//
// Created by yitzk on 9/15/2026.
//

#ifndef HASHTABLE_H
#define HASHTABLE_H
#include <cstdint>
#include <memory>
#include <vector>

# define NUM_ELEMENTS_IN_ARRAY 32

struct cyclicArray
{
    cyclicArray(uint32_t* array_ptr, uint32_t array_val): array_ptr(array_ptr), array_val(array_val)
    {
    }

    uint32_t* array_ptr;
    uint32_t array_val;

    uint8_t index_in_array = 0;
    bool array_full = false;

    void add_elem(uint32_t new_elem)
    {
        array_ptr[index_in_array] = new_elem;
        index_in_array++;
        if (index_in_array == NUM_ELEMENTS_IN_ARRAY)
        {
            index_in_array = 0;
            array_full = true;
        }
    }
};

class hashTable
{
public:
    explicit hashTable(int power_of_two_size);

    cyclicArray* find(uint32_t val);

    void add(uint32_t val);

    void clear();

private:
    [[nodiscard]] uint32_t hashFunction(uint32_t val) const;


    uint32_t hash_table_power_of_two_size;

    std::vector<std::unique_ptr<cyclicArray>> hash_table;

    std::vector<uint32_t> super_cyclic_array;

    static const uint32_t golden_ratio = 2654435769;

    static const int num_looks = 8;

    friend class TestHashTable;
};

#endif //HASHTABLE_H