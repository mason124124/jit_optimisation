#include <iostream>
#include <chrono>

// We use 'volatile' to prevent the compiler from optimizing 
// the check away at compile-time (simulating a runtime config flag)
volatile bool SAFETY_CHECK_ENABLED = true;

int runStrategy(int input) {
    if (SAFETY_CHECK_ENABLED) {
        return input + 1;
    } else {
        return input + 2;
    }
}

int main() {
    long long sum = 0;
    int iteration = 2000000000;
    
    // --- MEASUREMENT PHASE ---
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iteration; ++i) {
        sum += runStrategy(i);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> elapsed = end - start;

    std::cout << "C++ Time: " << elapsed.count() << " us" << std::endl;
    std::cout << "Check: " << sum << std::endl;
    
    return 0;
}