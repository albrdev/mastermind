#include "ProductionCode.h"
#include "unity.h"
#include "unity_fixture.h"
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <stdio.h>

TEST_GROUP(ProductionCode);

TEST_SETUP(ProductionCode)
{
    srand((unsigned int)time(NULL));
}

TEST_TEAR_DOWN(ProductionCode)
{
}

static bool checkUniques(const char* const arr, const size_t count)
{
    size_t i, j;
    size_t end = count - 1;

    // Loop through array and check if every element is unique
    for(i = 0; i < end; i++)
    {
        for(j = i + 1; j < count; j++)
        {
            if(arr[j] == arr[i])
            {
                return false;
            }
        }
    }

    return true;
}

static void printArray(const char* const arr, const size_t count)
{
    size_t i;

    for(i = 0; i < count; i++)
    {
        fprintf(stderr, "arr[%zu] = %c\n", i, arr[i]);
    }
}


TEST(ProductionCode, ShuffleSort)
{
    char arr[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    size_t count = sizeof(arr) / sizeof(*arr);

    shuffleSort(arr, count);
    bool res = checkUniques(arr, count);

    printArray(arr, count);
    
    TEST_ASSERT_TRUE(res);
}
