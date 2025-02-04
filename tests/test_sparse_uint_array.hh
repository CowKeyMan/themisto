#pragma once

#include "setup_tests.hh"
#include <gtest/gtest.h>
#include "globals.hh"
#include "sbwt/globals.hh"
#include "coloring/Sparse_Uint_Array.hh"

using namespace sbwt;

TEST(TEST_SPARSE_UINT_ARRAY, random_test){
    int64_t length = 1000;
    int64_t max_value = 8; // power of 2
    Sparse_Uint_Array_Builder builder(length, 2048, 3);
    
    int64_t NOTFOUND = 1e9;
    vector<int64_t> reference(length, NOTFOUND);
    srand(12345);

    // Set 500 random values
    for(int64_t i = 0; i < 500; i++){
        
        int64_t index = rand() % length;
        int64_t value = rand() % (max_value+1);

        builder.add(index,value);

        reference[index] = min(reference[index], value);
        // ^ The comment in builder.add says that if multiple values go to the same index, the smallest values is kept
    }

    Sparse_Uint_Array A = builder.finish();

    // Check
    for(int64_t i = 0; i < length; i++){
        logger << i << " " << reference[i] << endl;
        if(reference[i] == NOTFOUND)
            ASSERT_EQ(A.get(i), -1);
        else
            ASSERT_EQ(A.get(i), reference[i]);
    }
}