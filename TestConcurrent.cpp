#include "LargeAtomic.h"
#include <cassert>
#include <thread>
#include <random>

void singleThreadedTest()
{
    LargeAtomic<int> concurrentStore;

    for(int i = 0; i < 100; ++i)
    {
        concurrentStore.store(i);
        assert(*concurrentStore.load() == i);

    }

    LargeAtomic<std::vector<int>> vectorStore;
    std::vector<int> v = {1,2,3};
    vectorStore.store(v);
    assert(*vectorStore.load() == v);
}

/*
    Verify that, if we write sequentially increasing numbers, that a reader thread sees sequentially increasing numbers.
*/
void multithreadedCorrectnessTest() {
    LargeAtomic<int> concurrentStore(0); // Start with an initial value

    bool readCorrectly = true;

    auto writer = [&concurrentStore]() {
        for (int i = 1; i <= 100; ++i) {
            concurrentStore.store(i);
        }
    };

    auto reader = [&concurrentStore, &readCorrectly]() {
        int previousValue = 0;
        for (int i = 1; i <= 100; ++i) {
            auto currentValue = concurrentStore.load();
            if (*currentValue < previousValue) {
                readCorrectly = false;
            }
            previousValue = *currentValue;
        }
    };

    std::thread writerThread(writer);
    std::thread readerThread(reader);

    writerThread.join();
    readerThread.join();

    assert(readCorrectly && "Reader observed an out-of-order value!");
}

// Stress test with multiple threads writing and reading concurrently.
void stressTest() {
    LargeAtomic<int> concurrentStore(1);

    const int numReaders = 10;
    const int iterations_per_thread = 1000;

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

            if (dis(gen) % 10 == 0) {
                std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
            }
        }
    };

    auto reader = [&concurrentStore]() {
        for (int i = 0; i < iterations_per_thread; ++i) {
            auto loadedValue = concurrentStore.load();

            assert(*loadedValue >= 1 && *loadedValue <= 100);

            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    };

    // Launch writer thread
    threads.emplace_back(writer);

    // Launch reader threads
    for (int i = 0; i < numReaders; ++i) {
        threads.emplace_back(reader);
    }

    for (auto& t : threads) {
        t.join();
    }
}
