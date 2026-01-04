#include <iostream>
#include <chrono>

int main() {

    constexpr long long ITERATIONS = 10'000'000'000LL;
    constexpr long long PHASE = ITERATIONS / 2;

    long long count = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (long long i = 0; i < ITERATIONS; ++i) {
        bool condition;

        if (i < PHASE) {
            // Phase 1: condition is TRUE ~1% of the time
            condition = (i % 100 == 0);
        } else {
            // Phase 2: condition is TRUE ~99% of the time
            condition = (i % 100 != 0);
        }

        if (condition) {
            count++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;

    std::cout << "Time (ms): " << elapsed.count() << '\n';
    std::cout << "Count: " << count << '\n';

    return 0;
}
