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

template<long Size>
struct LargeStruct
{
  std::array<char, Size> data{};
};

template <typename T, typename IntRegister>
void testRegister(IntRegister &&concurrentStore) {
  const int numReaders = 1;
  const int iterations_per_thread = 100000;

  std::vector<std::thread> threads;

  // Lambda function for threads to perform random writes
  auto writer = [&concurrentStore]() {
    for (int i = 0; i < iterations_per_thread; ++i) {
      concurrentStore.store(T{});
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
  using testing_t = LargeStruct<1096>;

  std::cout << "Naive locking register: " << std::endl;
  std::cout << duration_cast<milliseconds>(benchmarkFunction([]() {
                 testRegister<testing_t>(LockingLargeAtomic<testing_t>());
               })).count()
            << "ms" << std::endl;

  std::cout << "Lock free LargeAtomic: " << std::endl;
  std::cout << duration_cast<milliseconds>(benchmarkFunction([]() {
                 testRegister<testing_t>(LargeAtomic<testing_t>());
               })).count()
            << "ms" << std::endl;
}
