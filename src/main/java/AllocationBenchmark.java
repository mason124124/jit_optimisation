import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class AllocationBenchmark {

  static class Order {

    long id;
    long price;
    long quantity;
  }

  // Prevent Dead Code Elimination
  static volatile long blackhole = 0;

  public static void main(String[] args) throws InterruptedException {
    int threadCount = 4;
    int allocationsPerThread = 10_000_000;

    System.out.println("Starting TLAB Benchmark...");
    System.out.println("Threads: " + threadCount);
    System.out.println("Allocations per Thread: " + allocationsPerThread);

    ExecutorService executor = Executors.newFixedThreadPool(threadCount);

    long start = System.nanoTime();

    for (int i = 0; i < threadCount; i++) {
      executor.submit(() -> {
        long sum = 0;
        for (int j = 0; j < allocationsPerThread; j++) {
          // ALLOCATION HAPPENS HERE
          // In Java, this is a "pointer bump" inside the TLAB.
          Order o = new Order();

          o.id = j;
          o.price = j * 10;
          sum += o.price; // Use the object so it isn't eliminated
        }
        updateBlackhole(sum);
      });
    }

    executor.shutdown();
    executor.awaitTermination(1, TimeUnit.MINUTES);

    long end = System.nanoTime();
    double durationMs = (end - start) / 1_000_000.0;

    System.out.printf("Total Time: %.2f ms%n", durationMs);
    System.out.printf("Throughput: %.2f million allocs/sec%n",
        (threadCount * allocationsPerThread) / (durationMs * 1000));
  }

  private static synchronized void updateBlackhole(long val) {
    blackhole += val;
  }
}
