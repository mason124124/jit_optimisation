#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>

// A dummy object representing a market order
struct Order {
    long id;
    long price;
    long quantity;
};

// Prevent Dead Code Elimination
std::atomic<long> blackhole(0);

void worker(int allocations) {
    long sum = 0;
    for (int j = 0; j < allocations; ++j) {
        // ALLOCATION HAPPENS HERE
        // Standard 'new' calls malloc. In multi-threaded code, 
        // this often involves lock contention or atomic overhead.
        Order* o = new Order();
        
        o->id = j;
        o->price = j * 10;
        sum += o->price;

        // We must manually delete. 
        // 'free' also involves overhead and synchronization.
        delete o;
    }
    blackhole += sum;
}

int main() {
    const int threadCount = 4;
    const int allocationsPerThread = 10000000;

    std::cout << "Starting Standard Allocator Benchmark...\n";
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < threadCount; ++i) {
        threads.emplace_back(worker, allocationsPerThread);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Total Time: " << duration.count() << " ms\n";
    double throughput = (threadCount * allocationsPerThread) / (duration.count() * 1000);
    std::cout << "Throughput: " << throughput << " million allocs/sec\n";

    return 0;
}