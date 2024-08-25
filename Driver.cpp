#include "TestConcurrent.h"

int main()
{
    singleThreadedTest();
    stressTest();
    multithreadedCorrectnessTest();
}
