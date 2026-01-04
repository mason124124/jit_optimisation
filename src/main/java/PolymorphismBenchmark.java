import java.util.Random;

/**
 * Demonstrates JVM JIT runtime optimisation on subtype polymorphism: - Hot call sites start interpreted, then compile (C1/C2). - With a MONOMORPHIC call site
 * (mostly one concrete Shape), HotSpot can devirtualize + inline. - When we later introduce more Shape subtypes (BI-/MEGAMORPHIC), the JVM may deopt +
 * recompile, and the inline-cache strategy changes (CHA + inline caching).
 * <p>
 * Run suggestions (optional, for “show the receipts”): javac JitPolymorphismDemo.java
 * <p>
 * # show compilation + inlining decisions: java -XX:+UnlockDiagnosticVMOptions -XX:+PrintCompilation -XX:+PrintInlining JitPolymorphismDemo
 * <p>
 * # additionally show deoptimisations: java -XX:+UnlockDiagnosticVMOptions -XX:+PrintCompilation -XX:+PrintInlining -XX:+TraceDeoptimization
 * JitPolymorphismDemo
 * <p>
 * Notes: - This is NOT a scientific benchmark. It's a demonstrator for JIT behaviour. - Keep it running long enough so HotSpot has time to optimise (default
 * thresholds).
 */
public final class PolymorphismBenchmark {

  // Prevent the JIT from dead-code-eliminating the whole loop.
  // (Volatile makes the result observable.)
  private static volatile long blackhole;

  // A small interface to force virtual/interface dispatch.
  interface Shape {

    int draw();          // some compute

    boolean isVisible(); // another virtual call
  }

  // --- Concrete implementations ---
  static final class Circle implements Shape {

    // Give the JIT something real to chew on; keep it simple and deterministic.
    private int x = 1;

    @Override
    public int draw() {
      // small arithmetic loop encourages inlining and scalar optimisations
      int v = x;
      v = v * 1664525 + 1013904223; // LCG-ish
      x = v;
      return v;
    }

    @Override
    public boolean isVisible() {
      // stable-ish predicate (branch friendly)
      return (x & 7) != 0; // true ~87.5%
    }
  }

  static final class Square implements Shape {

    private int y = 2;

    @Override
    public int draw() {
      int v = y;
      v ^= (v << 13);
      v ^= (v >>> 17);
      v ^= (v << 5); // xorshift
      y = v;
      return v;
    }

    @Override
    public boolean isVisible() {
      return (y & 15) != 0; // true ~93.75%
    }
  }

  static final class Triangle implements Shape {

    private int z = 3;

    @Override
    public int draw() {
      int v = z;
      v = (v * 31) + (v >>> 3) + 7;
      z = v;
      return v;
    }

    @Override
    public boolean isVisible() {
      return (z & 3) != 0; // true ~75%
    }
  }

  // --- Workload driver ---
  private static long drawLoop(Shape[] shapes, int rounds) {
    long acc = 0;
    for (int r = 0; r < rounds; r++) {
      // tight loop: classic subtype polymorphism site: shapes[i].draw()
      for (int i = 0; i < shapes.length; i++) {
        Shape s = shapes[i];

        // Two virtual/interface calls back-to-back:
        // If monomorphic, HotSpot can inline both and optimise across them.
        int v = s.draw();
        if (s.isVisible()) {
          acc += (v & 0xFFFF);
        } else {
          acc -= (v & 0xFF);
        }
      }
    }
    return acc;
  }

  private static Shape[] mostlyOneType(int size, Shape hot, Shape cold, int hotPercent, long seed) {
    Random rnd = new Random(seed);
    Shape[] a = new Shape[size];
    for (int i = 0; i < size; i++) {
      a[i] = (rnd.nextInt(100) < hotPercent) ? hot : cold;
    }
    return a;
  }

  private static Shape[] mixThreeTypes(int size, Shape a, Shape b, Shape c, long seed) {
    Random rnd = new Random(seed);
    Shape[] arr = new Shape[size];
    for (int i = 0; i < size; i++) {
      int t = rnd.nextInt(100);
      if (t < 70) {
        arr[i] = a;      // 70%
      } else if (t < 90) {
        arr[i] = b; // 20%
      } else {
        arr[i] = c;             // 10%
      }
    }
    return arr;
  }

  private static void phase(String name, Shape[] shapes, int rounds) {
    long t0 = System.nanoTime();
    long res = drawLoop(shapes, rounds);
    long t1 = System.nanoTime();

    blackhole ^= res; // make it observable

    double ms = (t1 - t0) / 1_000_000.0;
    System.out.printf("%-28s  time(ms)=%.3f  result=%d  blackhole=%d%n", name, ms, res, blackhole);
  }

  public static void main(String[] args) {
    final int size = 1 << 16; // 65536 elements; enough to keep call site hot
    final int warmRounds = 20000; // adjust if your machine is very fast/slow
    final int measureRounds = 20000;

    Shape circle = new Circle();
    Shape square = new Square();
    Shape triangle = new Triangle();

    // Phase 1: monomorphic (Circle dominates heavily).
    // HotSpot often devirtualizes + inlines aggressively here.
    Shape[] mono = mostlyOneType(size, circle, square, 100, 1);

    // Phase 2: Bimorphic-ish (Circle still dominates, but Square appears more).
    Shape[] bi = mostlyOneType(size, circle, square, 80, 2);

    // Phase 3: More polymorphic (3 types).
    // Call site may become megamorphic-ish depending on distribution; inlining may be curtailed,
    // inline caches change shape; deoptimisation/recompilation can occur.
    Shape[] poly = mixThreeTypes(size, circle, square, triangle, 3);

    System.out.println("=== Warm-up (let the JIT observe + compile) ===");
    phase("warmup monomorphic 100", mono, warmRounds);
    phase("warmup bimorphic   80/20", bi, warmRounds);
    phase("warmup polymorphic 70/20/10", poly, warmRounds);

    System.out.println("\n=== Measure (steady-state-ish) ===");
    phase("measure monomorphic 100", mono, measureRounds);
    phase("measure bimorphic   80/20", bi, measureRounds);
    phase("measure polymorphic 70/20/10", poly, measureRounds);

    System.out.println("\nTip: run with -XX:+PrintInlining and look for inlining at call sites:");
    System.out.println("  shapes[i].draw() and shapes[i].isVisible()");
    System.out.println("Then introduce more subtypes and watch inlining decisions change (and possible deopts).");
  }
}
