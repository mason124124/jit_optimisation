public class JITBranchProof {

  // volatile ensures visibility, but JIT will still speculate it's constant
  static volatile boolean SAFETY_CHECK = true;

  public static void main(String[] args) {
    int sum = 0;

    // 1. WARMUP (Training Phase)
    // Run enough times to trigger C2 compilation (usually ~10k iterations)
    // The JIT sees SAFETY_CHECK is always TRUE, so it recompiles 'work'
    // effectively deleting the 'if' statement.
    for (int i = 0; i < 20_000; i++) {
      sum += work(i);
    }

    System.out.println("Warmup complete. JIT has likely removed the branch.");

    long start = System.nanoTime();

    for (int i = 0; i < 20_000; i++) {
      sum += work(i);
    }

    long end = System.nanoTime();

    long total = end - start;
    long perIter = total / 20_000;
    System.out.println("Latency of the 'Normal' iteration: " + total + " ns" + " (" + perIter + " ns per iteration)");
    // 2. THE TRAP (The "Gotcha" Moment)
    // We flip the flag. The current machine code CANNOT handle this
    // because the 'else' path physically does not exist in the optimized code.
    SAFETY_CHECK = false;

    start = System.nanoTime();

    // This single line triggers the "Uncommon Trap"
    sum += work(1);

    end = System.nanoTime();

    // 3. REPORTING
    System.out.println("Latency of the 'Trap' iteration: " + (end - start) + " ns");
    System.out.println("Normal latency reference: ~10-100 ns");

    // 4. warming up JIT again
    for (int i = 0; i < 2_000_000; i++) {
      work(i);
    }

    System.out.println("JIT has recompiled 'work' again.");

    // 5. benchmark again
    start = System.nanoTime();

    // This single line triggers the "Uncommon Trap"
    for (int i = 0; i < 20_000; i++) {
      sum += work(i);
    }

    end = System.nanoTime();
    total = end - start;
    perIter = total / 20_000;
    System.out.println("Latency of the 'Normal' iteration: " + total + " ns" + " (" + perIter + " ns per iteration)");
  }

  public static int work(int input) {
    if (SAFETY_CHECK) {
      return input + 1;
    } else {
      return input + 2;
    }
  }
}
