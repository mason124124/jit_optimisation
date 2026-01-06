// allocation_benchmark.cpp
//
// Compare C++ allocation strategies:
//  1) plain new/delete
//  2) a simple pool allocator (TLAB-like bump allocation + free list)
//
// Build:
//   g++ -O3 -march=native -std=c++17 allocation_benchmark.cpp -o allocation_benchmark
// Run:
//   ./allocation_benchmark
//
// Notes:
// - This is a demonstrator. Real allocators (jemalloc/tcmalloc) are more sophisticated.
// - We FORCE allocations to be "real" by storing pointers in a ring buffer (escape).
// - We overwrite entries so old objects are deleted / recycled quickly.

#include <cstdint>
#include <cstdio>
#include <chrono>
#include <vector>
#include <cstdlib>
#include <new>

static volatile std::uint64_t blackhole = 0;

struct Msg {
    std::uint64_t a;
    std::uint64_t b;
};

// -------------------------------
// Strategy A: plain new/delete
// -------------------------------
static std::uint64_t run_new_delete(std::size_t iters, std::vector<Msg*>& ring) {
    const std::size_t mask = ring.size() - 1;
    std::uint64_t acc = 0;

    for (std::size_t i = 0; i < iters; ++i) {
        const std::size_t idx = i & mask;

        // delete previous object in this slot (object becomes "garbage")
        Msg* old = ring[idx];
        if (old) {
            delete old;
        }

        // allocate a new one
        Msg* m = new Msg{ i, i ^ 0x9E3779B97F4A7C15ULL };
        ring[idx] = m; // force escape

        acc += (m->a & 0xFF) + (m->b & 0xFF);
    }

    return acc;
}

// --------------------------------------------
// Strategy B: pool allocator (TLAB-like style)
// --------------------------------------------
// Very small, simple pool:
// - allocate big chunks
// - bump pointer for fresh allocations
// - recycle objects via free list
//
// This mimics the spirit of TLAB: fast local allocation for the common case.
// Unlike Java, *we* must define the reclamation strategy (free list here).
class MsgPool {
public:
    explicit MsgPool(std::size_t chunkCount)
        : chunkCount_(chunkCount) {
        allocate_chunk();
    }

    Msg* alloc(std::uint64_t a, std::uint64_t b) {
        // First try free list (recycling)
        if (free_) {
            Node* n = free_;
            free_ = free_->next;
            Msg* m = reinterpret_cast<Msg*>(n);
            m->a = a;
            m->b = b;
            return m;
        }

        // Otherwise bump allocate from current chunk
        if (cursor_ == end_) {
            allocate_chunk();
        }

        Msg* m = cursor_++;
        m->a = a;
        m->b = b;
        return m;
    }

    void free(Msg* m) {
        // Push into free list (O(1))
        Node* n = reinterpret_cast<Node*>(m);
        n->next = free_;
        free_ = n;
    }

    ~MsgPool() {
        for (void* p : chunks_) std::free(p);
    }

private:
    struct Node { Node* next; };

    void allocate_chunk() {
        // Allocate raw memory for chunkCount_ Msg objects
        void* mem = std::aligned_alloc(alignof(Msg), chunkCount_ * sizeof(Msg));
        if (!mem) throw std::bad_alloc();

        chunks_.push_back(mem);
        cursor_ = reinterpret_cast<Msg*>(mem);
        end_    = cursor_ + chunkCount_;
    }

    std::size_t chunkCount_;
    std::vector<void*> chunks_;

    Msg* cursor_ = nullptr;
    Msg* end_    = nullptr;

    Node* free_  = nullptr;
};

static std::uint64_t run_pool(std::size_t iters, std::vector<Msg*>& ring, MsgPool& pool) {
    const std::size_t mask = ring.size() - 1;
    std::uint64_t acc = 0;

    for (std::size_t i = 0; i < iters; ++i) {
        const std::size_t idx = i & mask;

        // "reclaim" previous object in slot by returning to pool
        Msg* old = ring[idx];
        if (old) {
            pool.free(old);
        }

        // allocate from pool (fast path: free list or bump pointer)
        Msg* m = pool.alloc(i, i ^ 0x9E3779B97F4A7C15ULL);
        ring[idx] = m; // force escape

        acc += (m->a & 0xFF) + (m->b & 0xFF);
    }

    return acc;
}

static void phase(const char* name, std::uint64_t (*fn)(std::size_t, std::vector<Msg*>&),
                  std::size_t iters, std::vector<Msg*>& ring) {
    auto t0 = std::chrono::high_resolution_clock::now();
    std::uint64_t res = fn(iters, ring);
    auto t1 = std::chrono::high_resolution_clock::now();

    blackhole ^= res;

    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double nsPerOp = (ms * 1e6) / static_cast<double>(iters);
    std::printf("%-12s time(ms)=%.3f  ns/op=%.2f  result=%llu  blackhole=%llu\n",
                name, ms, nsPerOp,
                (unsigned long long)res,
                (unsigned long long)blackhole);
}

int main() {
    const std::size_t ringSize = 1 << 20; // power of 2
    std::vector<Msg*> ring(ringSize, nullptr);

    const std::size_t warmIters = 30'000'000;
    const std::size_t measIters = 150'000'000;

    std::puts("C++ allocation benchmark: new/delete vs pool (TLAB-like)\n");

    // Warm-up caches and branch predictor
    phase("warmup new", run_new_delete, warmIters, ring);
    phase("measure new", run_new_delete, measIters, ring);

    // Clean ring (delete remaining to avoid leaking between tests)
    for (Msg*& p : ring) { delete p; p = nullptr; }

    // Pool: chunkCount controls how many Msg per chunk allocation.
    MsgPool pool(/*chunkCount=*/1 << 18); // 262k objects per chunk

    // Run pool version
    auto pool_adapter = [&](std::size_t it, std::vector<Msg*>& r) -> std::uint64_t {
        return run_pool(it, r, pool);
    };

    // Can't pass capturing lambda to the existing phase signature easily; do inline timing:
    auto run_pool_phase = [&](const char* label, std::size_t iters) {
        auto t0 = std::chrono::high_resolution_clock::now();
        std::uint64_t res = run_pool(iters, ring, pool);
        auto t1 = std::chrono::high_resolution_clock::now();
        blackhole ^= res;

        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        double nsPerOp = (ms * 1e6) / static_cast<double>(iters);
        std::printf("%-12s time(ms)=%.3f  ns/op=%.2f  result=%llu  blackhole=%llu\n",
                    label, ms, nsPerOp,
                    (unsigned long long)res,
                    (unsigned long long)blackhole);
    };

    run_pool_phase("warmup pool", warmIters);
    run_pool_phase("measure pool", measIters);

    // Return any remaining objects in ring to pool (optional)
    for (Msg*& p : ring) { if (p) { pool.free(p); p = nullptr; } }

    return 0;
}
