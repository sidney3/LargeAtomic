#include "Benchmark.h"
#include "TestConcurrent.h"

int main() {
  singleThreadedTest();
  stressTest();
  multithreadedCorrectnessTest();
  benchmark();
}
