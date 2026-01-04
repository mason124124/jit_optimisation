/**
 * Compare Java allocations with TLAB enabled vs disabled.
 * <p>
 * Run: javac TLABBenchmark.java
 * <p>
 * TLAB ON (default): java -Xms2g -Xmx2g TLABBenchmark
 * <p>
 * TLAB OFF: java -Xms2g -Xmx2g -XX:-UseTLAB TLABBenchmark
 * <p>
 * Optional (to see GC): -Xlog:gc
 * <p>
 * Key trick: - We FORCE allocation to be real by storing objects into a ring buffer (escape). - We overwrite entries so objects die young (young-gen churn).
 */
public final class AllocationBenchmark2 {

  static final class Msg {

    final long a;
    final long b;

    Msg(long a, long b) {
      this.a = a;
      this.b = b;
    }
  }

  private static volatile long blackhole;
  private static Msg[] ring;

  private static long runOnce(int iterations, int ringMask) {
    long acc = 0;
    Msg[] r = ring;

    for (int i = 0; i < iterations; i++) {
      Msg m = new Msg(i, i ^ 0x9E3779B97F4A7C15L);
      r[i & ringMask] = m;             // force escape
      acc += (m.a & 0xFF) + (m.b & 0xFF);
    }
    return acc;
  }

  private static void phase(String name, int iterations, int ringMask) {
    long t0 = System.nanoTime();
    long res = runOnce(iterations, ringMask);
    long t1 = System.nanoTime();

    blackhole ^= res;

    double ms = (t1 - t0) / 1_000_000.0;
    double nsPerOp = (double) (t1 - t0) / (double) iterations;
    System.out.printf("%-8s  time(ms)=%.3f  ns/op=%.2f  result=%d  blackhole=%d%n",
        name, ms, nsPerOp, res, blackhole);
  }

  public static void main(String[] args) {
    final int ringSize = 1 << 20;        // must be power of 2
    final int ringMask = ringSize - 1;
    ring = new Msg[ringSize];

    final int warmIters = 30_000_000;
    final int measIters = 150_000_000;

    System.out.println("Java allocation benchmark (TLAB on/off).");
    System.out.println("Try: -XX:-UseTLAB  (plus optional -Xlog:gc)\n");

    phase("warmup", warmIters, ringMask);
    phase("measure", measIters, ringMask);
  }
}
