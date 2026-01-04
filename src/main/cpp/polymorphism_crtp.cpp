#include <cstdint>
#include <cstdio>
#include <vector>
#include <random>
#include <chrono>

// Prevent Dead Code Elimination
static volatile std::uint64_t blackhole = 0;

// --- 1. The CRTP Base Class ---
// "Derived" is a template parameter. The compiler creates a unique 
// Shape base class for every specific child (Shape<Circle>, Shape<Square>).
template <typename Derived>
struct Shape {
    // No 'virtual' keyword. No vtable.
    // The compiler hard-codes the call to the Derived implementation.
    std::int32_t draw() noexcept {
        return static_cast<Derived*>(this)->drawImpl();
    }

    bool isVisible() noexcept {
        return static_cast<Derived*>(this)->isVisibleImpl();
    }
};

// --- 2. Concrete Implementations ---
// Notice we inherit from Shape<Circle>, not just Shape.
struct Circle final : Shape<Circle> {
    std::int32_t x = 1;

    // We name the method 'drawImpl' to distinguish from the base 'draw' wrapper
    std::int32_t drawImpl() noexcept {
        std::int32_t v = x;
        v = v * 1664525 + 1013904223;
        x = v;
        return v;
    }

    bool isVisibleImpl() noexcept {
        return (x & 7) != 0;
    }
};

struct Square final : Shape<Square> {
    std::int32_t y = 2;

    std::int32_t drawImpl() noexcept {
        std::int32_t v = y;
        v ^= (v << 13);
        v ^= (v >> 17);
        v ^= (v << 5);
        y = v;
        return v;
    }

    bool isVisibleImpl() noexcept {
        return (y & 15) != 0;
    }
};

// --- 3. The Driver (Templated) ---
// Since Circle and Square are different types, we need a template function
// that can accept a vector of ANY shape type.
template <typename T>
static std::uint64_t drawLoop(std::vector<T>& shapes, int rounds) noexcept {
    std::uint64_t acc = 0;
    for (int r = 0; r < rounds; ++r) {
        // Direct access. The compiler effectively pastes the code of 
        // drawImpl() right here. Zero function call overhead.
        for (auto& s : shapes) {
            std::int32_t v = s.draw(); 
            if (s.isVisible()) {
                acc += static_cast<std::uint32_t>(v) & 0xFFFFu;
            } else {
                acc -= static_cast<std::uint32_t>(v) & 0xFFu;
            }
        }
    }
    return acc;
}

// Helper to fill a vector
template <typename T>
std::vector<T> createShapes(std::size_t size) {
    return std::vector<T>(size); 
}

template <typename T>
static void measure(const char* name, std::vector<T>& shapes, int rounds) {
    auto t0 = std::chrono::high_resolution_clock::now();
    
    std::uint64_t res = drawLoop(shapes, rounds);
    
    auto t1 = std::chrono::high_resolution_clock::now();
    blackhole ^= res;

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("%-30s  time(ms)=%.3f  result=%llu\n", 
                name, ms, static_cast<unsigned long long>(res));
}

int main() {
    const std::size_t size = 1u << 16; // 65536
    const int warmRounds = 20000;
    const int measureRounds = 20000;

    // Note: We cannot mix Circles and Squares in one vector anymore.
    // We must measure them separately.
    std::vector<Circle> circles = createShapes<Circle>(size);
    std::vector<Square> squares = createShapes<Square>(size);

    std::puts("=== Warm-up ===");
    measure("warmup CRTP (Circle)", circles, warmRounds);
    measure("warmup CRTP (Square)", squares, warmRounds);

    std::puts("\n=== Measure ===");
    measure("measure CRTP (Circle)", circles, measureRounds);
    measure("measure CRTP (Square)", squares, measureRounds);

    return 0;
}