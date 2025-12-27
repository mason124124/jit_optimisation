public class BranchPredictionBenchmark {

  public static void main(String[] args) {
    final long ITERATIONS = 10_000_000_000L;
    final long PHASE = ITERATIONS / 2;

    long count = 0;

    final long start = System.nanoTime();

    for (long i = 0; i < ITERATIONS; i++) {
      boolean condition;

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

    final long end = System.nanoTime();
    final double elapsedMs = (end - start) / 1_000_000.0;

    System.out.println("Time (ms): " + elapsedMs);
    System.out.println("Count: " + count);
  }
}
