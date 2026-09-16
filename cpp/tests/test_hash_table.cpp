//
// Created by yitzk on 9/15/2026.
//
#include<gtest/gtest.h>
#include<../include/hashTable.h>

class TestHashTable : public ::testing::Test
{
protected:
   hashTable hash_table = hashTable(18);


};

TEST_F(TestHashTable, CyclicArrayBasicTest)
{
   uint32_t array[NUM_ELEMENTS_IN_ARRAY];

   uint32_t array_val = 34576;
   cyclicArray cyclic_array(array, array_val);

   // test init fields
   EXPECT_EQ(cyclic_array.array_full, false);
   EXPECT_EQ(cyclic_array.array_ptr, array);
   EXPECT_EQ(cyclic_array.array_val, array_val);
   EXPECT_EQ(cyclic_array.index_in_array,0);

   // fill array to the top
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++) cyclic_array.add_elem(i);

   // test filling array
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++) EXPECT_EQ(array[i], i);

   // test if the array fields have the expected value
   EXPECT_EQ(cyclic_array.array_full, true);
   EXPECT_EQ(cyclic_array.index_in_array, 0);

   // test the cyclic property
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY >> 1; i++) cyclic_array.add_elem(-i);
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++)
   {
      if(i < NUM_ELEMENTS_IN_ARRAY >> 1) EXPECT_EQ(array[i], -i);
      else  EXPECT_EQ(array[i], i);
   }
   EXPECT_EQ(cyclic_array.array_full, true);
   EXPECT_EQ(cyclic_array.index_in_array, NUM_ELEMENTS_IN_ARRAY >> 1 );
}

TEST_F(TestHashTable, CyclicArrayEdgeCasesTest)
{

   auto* array = (uint32_t*) malloc(NUM_ELEMENTS_IN_ARRAY * (2 << 18) * sizeof(uint32_t));

   uint32_t array_val = 34576;
   cyclicArray cyclic_array(array + (NUM_ELEMENTS_IN_ARRAY * 2<<12),array_val);
   auto* new_array =  array + (NUM_ELEMENTS_IN_ARRAY * 2<<12);
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++) cyclic_array.add_elem(i);

   // test filling array
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY; i++)
   {
      EXPECT_EQ(new_array[i], i);
   }

   // test cycle array many cycles
   for(int i = 0; i < NUM_ELEMENTS_IN_ARRAY * 100; i++) cyclic_array.add_elem(i);

   for(int j = 0, i = NUM_ELEMENTS_IN_ARRAY * 99; i < NUM_ELEMENTS_IN_ARRAY * 100; i++, j++)
   {
      EXPECT_EQ(new_array[j], i);
   }

   free(array);
}