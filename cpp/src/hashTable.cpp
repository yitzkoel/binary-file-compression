//
// Created by yitzk on 9/15/2026.
//
#include "hashTable.h"

hashTable::hashTable(int power_of_two_size):
    hash_table(1 << power_of_two_size),
    super_cyclic_array(NUM_ELEMENTS_IN_ARRAY * (1 << power_of_two_size)),
    hash_table_power_of_two_size(power_of_two_size)
{
}

cyclicArray* hashTable::find(uint32_t val)
{
    uint32_t hash_val = hashFunction(val);
    int counter = 0;
    while (counter < num_looks)
    {
        // if there is no element then it is not here
        if(hash_table[hash_val] == nullptr) return nullptr;
        // if we found it then break the loop
        if(hash_table[hash_val]->array_val == val) break;

        // increment the indexs
        hash_val++;
        counter++;
        hash_val = hash_val & ((1 << hash_table_power_of_two_size) - 1); // take modulo size of table to not overflow the vector

        // if we searched more that num_looks then we dont keep searching
        if (counter == num_looks)  return nullptr;
    }
    return hash_table[hash_val].get();
}

void hashTable::add(uint32_t val)
{
    uint32_t hash_val = hashFunction(val);
    int counter = 0;
    while (hash_table[hash_val] != nullptr)
    {
        // this val already is in the hash map
        if(hash_table[hash_val]->array_val == val) return;

        hash_val++;
        hash_val = hash_val & ((1 << hash_table_power_of_two_size) - 1); // take modulo size of table to not overflow the vector
        counter++;
        if (counter == num_looks)
        {
            hash_val = hashFunction(val);
            break;
        }
    }
    uint32_t* ptr_to_array = super_cyclic_array.data() + (hash_val * NUM_ELEMENTS_IN_ARRAY);
    hash_table[hash_val] = std::make_unique<cyclicArray>(ptr_to_array, val);
}

void hashTable::clear()
{
    // clears the vector of the hash table
    std::ranges::fill(hash_table, nullptr);
}

uint32_t hashTable::hashFunction(uint32_t val) const
{
    return (val * golden_ratio) >> (32 - hash_table_power_of_two_size);
}