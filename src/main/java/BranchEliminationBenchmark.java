public class BranchEliminationBenchmark {

  // Volatile allows us to change this at runtime,
  // but JIT will still try to optimize the loop "optimistically"
  static volatile boolean SAFETY_CHECK_ENABLED = true;

  public static void main(String[] args) {
    long sum = 0;

    // --- MEASUREMENT PHASE ---
    long start = System.nanoTime();

    // We run a massive loop with the check ENABLED
    int iteration = 2_000_000_000;
    for (int i = 0; i < iteration; i++) {
      sum += runStrategy(i);
    }

    long end = System.nanoTime();
    System.out.printf("Java Time: %.6f us%n", (end - start) / 1_000.0);
    System.out.println("Check: " + sum);
  }

  // The JIT will eventually realize SAFETY_CHECK_ENABLED is constant
  // for long periods and may "Inline" this method and remove the IF entirely.
  public static int runStrategy(int input) {
    if (SAFETY_CHECK_ENABLED) {
      return input + 1; // Hot Path
    } else {
      return input + 2; // Cold Path
    }
  }
}
