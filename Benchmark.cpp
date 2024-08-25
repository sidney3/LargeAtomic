#include "LargeAtomic.h"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

using namespace std::chrono;

/*
    Naive atomic register for us to compare against
*/
template <typename T> class LockingLargeAtomic {
private:
  std::mutex mtx;
  T elt;

public:
  LockingLargeAtomic(const T &init = T{}) : elt(init) {}

  const T *load() {
    std::lock_guard lk{mtx};
    return &elt;
  }
  template <typename... Args> void store(Args &&...args) {
    std::lock_guard lk{mtx};
    elt = T{std::forward<Args>(args)...};
  }
};

template <typename Fn> auto benchmarkFunction(Fn &&fn) {
  auto start = system_clock::now();

  fn();

  return system_clock::now() - start;
}

template <typename IntRegister>
void testRegister(IntRegister &&concurrentStore) {
  const int numReaders = 1;
  const int iterations_per_thread = 10000;

  std::vector<std::thread> threads;

  // Random generator for introducing random delays
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1, 100);

  // Lambda function for threads to perform random writes
  auto writer = [&concurrentStore, &gen, &dis]() {
    for (int i = 0; i < iterations_per_thread; ++i) {
      int newValue = dis(gen); // Generate a random value to store
      concurrentStore.store(newValue);
    }
  };

  auto reader = [&concurrentStore]() {
    for (int i = 0; i < iterations_per_thread; ++i) {
      concurrentStore.load();
    }
  };

  // Launch writer thread
  threads.emplace_back(writer);

  // Launch reader threads
  for (int i = 0; i < numReaders; ++i) {
    threads.emplace_back(reader);
  }

  for (auto &t : threads) {
    t.join();
  }
}

void benchmark() {
  std::cout << "Naive locking register: " << std::endl;
  std::cout << duration_cast<nanoseconds>(benchmarkFunction([]() {
                 testRegister(LockingLargeAtomic<int>());
               })).count()
            << "ns" << std::endl;

  std::cout << "Lock free LargeAtomic: " << std::endl;
  std::cout << duration_cast<nanoseconds>(benchmarkFunction([]() {
                 testRegister(LargeAtomic<int>());
               })).count()
            << "ns" << std::endl;
}
