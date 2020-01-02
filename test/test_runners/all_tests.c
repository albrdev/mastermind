#include "unity_fixture.h"

static void RunAllTests(void)
{
    RUN_TEST_GROUP(ProductionCode);
}

// Problem when running executable. May be this issue: https://github.com/ThrowTheSwitch/Unity/issues/402
int main(int argc, const char * argv[])
{
    //return UnityMain(argc, argv, RunAllTests);
    RunAllTests();
    return 0;
}
