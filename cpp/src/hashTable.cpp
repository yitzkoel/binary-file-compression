//
// Created by yitzk on 9/15/2026.
//
#include "hashTable.h"

hashTable::hashTable(int power_of_two_size):
    hash_table(2 << power_of_two_size),
    super_cyclic_array(NUM_ELEMENTS_IN_ARRAY * (2 << power_of_two_size)),
    hash_table_power_of_two_size(power_of_two_size)
{
}

cyclicArray* hashTable::find(uint32_t index)
{
    uint32_t hash_val = hashFunction(index);
    int counter = 0;
    while (hash_table[hash_val] == nullptr || hash_table[hash_val]->array_val != index)
    {
        hash_val++;
        counter++;
        if (counter == num_looks)
        {
            return nullptr;
        }
    }
    return hash_table[hash_val].get();
}

void hashTable::add(uint32_t index)
{
    uint32_t hash_val = hashFunction(index);
    int counter = 0;
    while (hash_table[hash_val] == nullptr)
    {
        hash_val++;
        counter++;
        if (counter == num_looks)
        {
            hash_val = hashFunction(index);
            break;
        }
    }
    uint32_t* ptr_to_array = super_cyclic_array.data() + (hash_val * NUM_ELEMENTS_IN_ARRAY);
    hash_table[hash_val] = std::make_unique<cyclicArray>(ptr_to_array, index);
}

void hashTable::clear()
{
    hash_table.clear();
}

uint32_t hashTable::hashFunction(uint32_t val) const
{
    return (val * golden_ratio) >> hash_table_power_of_two_size;
}