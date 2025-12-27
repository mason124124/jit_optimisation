// jit_polymorphism_demo.cpp
//
// C++ analogue of the Java “Shape[] + virtual draw()/isVisible()” demo.
// Purpose: show how AOT + virtual dispatch behaves under:
//   1) monomorphic (almost all one subtype)
//   2) bimorphic
//   3) polymorphic
//
// This is NOT a scientific benchmark; it’s a runnable demonstrator.
//
// Build (GCC/Clang):
//   g++ -O3 -march=native -std=c++20 jit_polymorphism_demo.cpp -o demo
//   ./demo
//
// Tip: also try clang++ and compare.
// Tip: LTO can change results:
//   g++ -O3 -march=native -flto -std=c++20 jit_polymorphism_demo.cpp -o demo

#include <cstdint>
#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
#include <memory>

static volatile std::uint64_t blackhole = 0; // prevent DCE

struct Shape {
    virtual ~Shape() = default;
    virtual std::int32_t draw() noexcept = 0;
    virtual bool isVisible() noexcept = 0;
};

struct Circle final : Shape {
    std::int32_t x = 1;

    std::int32_t draw() noexcept override {
        // simple, deterministic arithmetic (keeps it CPU-bound)
        std::int32_t v = x;
        v = v * 1664525 + 1013904223; // LCG-ish
        x = v;
        return v;
    }

    bool isVisible() noexcept override {
        return (x & 7) != 0; // true ~87.5%
    }
};

struct Square final : Shape {
    std::int32_t y = 2;

    std::int32_t draw() noexcept override {
        std::int32_t v = y;
        v ^= (v << 13);
        v ^= (v >> 17);
        v ^= (v << 5); // xorshift
        y = v;
        return v;
    }

    bool isVisible() noexcept override {
        return (y & 15) != 0; // true ~93.75%
    }
};

struct Triangle final : Shape {
    std::int32_t z = 3;

    std::int32_t draw() noexcept override {
        std::int32_t v = z;
        v = (v * 31) + (v >> 3) + 7;
        z = v;
        return v;
    }

    bool isVisible() noexcept override {
        return (z & 3) != 0; // true ~75%
    }
};

static std::uint64_t drawLoop(Shape* const* shapes, std::size_t n, int rounds) noexcept {
    std::uint64_t acc = 0;
    for (int r = 0; r < rounds; ++r) {
        for (std::size_t i = 0; i < n; ++i) {
            Shape* s = shapes[i];
            // two virtual calls back-to-back:
            // in a monomorphic world, a JIT might devirtualize+inline via runtime profiling.
            // in C++, the compiler has limited ability here unless it can prove the dynamic type.
            std::int32_t v = s->draw();
            if (s->isVisible()) {
                acc += static_cast<std::uint32_t>(v) & 0xFFFFu;
            } else {
                acc -= static_cast<std::uint32_t>(v) & 0xFFu;
            }
        }
    }
    return acc;
}

static std::vector<Shape*> mostlyOneType(
        std::size_t size,
        Shape* hot,
        Shape* cold,
        int hotPercent,
        std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(0, 99);
    std::vector<Shape*> a;
    a.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        a.push_back(dist(rng) < hotPercent ? hot : cold);
    }
    return a;
}

static std::vector<Shape*> mixThreeTypes(
        std::size_t size,
        Shape* a,
        Shape* b,
        Shape* c,
        std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(0, 99);
    std::vector<Shape*> arr;
    arr.reserve(size);
    for (std::size_t i = 0; i < size; ++i) {
        int t = dist(rng);
        if (t < 70) arr.push_back(a);        // 70%
        else if (t < 90) arr.push_back(b);   // 20%
        else arr.push_back(c);               // 10%
    }
    return arr;
}

static void phase(const char* name, const std::vector<Shape*>& shapes, int rounds) {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::uint64_t res = drawLoop(shapes.data(), shapes.size(), rounds);
    auto t1 = std::chrono::high_resolution_clock::now();

    blackhole ^= res;

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::printf("%-30s  time(ms)=%.3f  result=%llu  blackhole=%llu\n",
                name, ms,
                static_cast<unsigned long long>(res),
                static_cast<unsigned long long>(blackhole));
}

int main() {
    const std::size_t size = 1u << 16; // 65536
    const int warmRounds = 20000;
    const int measureRounds = 20000;

    // Keep objects alive and stable (no allocations in hot loop).
    Circle circle;
    Square square;
    Triangle triangle;

    // Three distributions:
    auto mono = mostlyOneType(size, &circle, &square, 99, 1);      // mostly Circle
    auto bi   = mostlyOneType(size, &circle, &square, 80, 2);      // Circle + Square
    auto poly = mixThreeTypes(size, &circle, &square, &triangle, 3); // 3 types

    std::puts("=== Warm-up (cache warming / steady state for CPU) ===");
    phase("warmup monomorphic 99/1", mono, warmRounds);
    phase("warmup bimorphic   80/20", bi, warmRounds);
    phase("warmup polymorphic 70/20/10", poly, warmRounds);

    std::puts("\n=== Measure ===");
    phase("measure monomorphic 99/1", mono, measureRounds);
    phase("measure bimorphic   80/20", bi, measureRounds);
    phase("measure polymorphic 70/20/10", poly, measureRounds);

    return 0;
}
