# Bitforge — BF-01 Independent Implementation and Performance Audit

Date: 2026-10-31
GitHub repository: https://github.com/adinax031-boop/bitforge-asm
Inspected upstream revision: 7b89590bd324e0c28e5869d0b7573313e82e9f02
Status: local reproduction only; **no GitHub modification**.

## Scope

Ported the current public repo's original `dispatch.c`, `kernels.S`, `bitforge.h`, and `test.c` to a local build directory from GitHub content reads. These are *locally reconstructed reference source copies*, not an authenticated `git clone` (external DNS unavailable). Built with GCC 14.2.0 on Linux x86-64 virtualized AMD EPYC 9V74. The test program prints:

```
PASS: randomized differential, tails, aliasing, guard pages, concurrent initialization; dispatch=avx2
```

Compared the reference implementations with a separately written optimized C `__builtin_popcountll` loop compiled with `-O3 -mpopcnt` and an experimental 512-bit VPOPCNTDQ intrinsic kernel, on the same virtual CPU. The candidate is a local experimental oracle/baseline; it is NOT merged into upstream and has not passed independent guard-page stress tests.

## Measured performance (single CPU affinity, median of seven trials; lower is better)

| Working set | Data | scalar Kernighan | Assembly POPCNT | Assembly AVX2 | GCC builtin | AVX-512 VPOPCNTDQ |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 64 KiB / input | Dense | 12.3627 | 0.5621 | 0.1921 | 0.6310 | **0.1518** |
| 64 KiB / input | Sparse | 0.6907 | 0.5662 | 0.2025 | 0.6637 | **0.1530** |
| 8 MiB / input | Dense | 16.7305 | 0.6420 | 0.2416 | 0.6929 | **0.2279** |
| 8 MiB / input | Sparse | 1.0630 | 0.6808 | **0.2185** | 0.6975 | 0.2301 |

Unit: nanoseconds per 64-bit input *word*, across two equal-sized input arrays; median of seven consecutive timer samples, repeated function calls. Benchmarks were pinned to CPU 0 using `taskset -c 0` on a shared/virtualized environment. 64 KiB/input means 128 KiB of total input; 8 MiB/input means 16 MiB total input. Dense inputs are pseudorandom; sparse inputs have additional bitmasking. Checksum values checked against independent builtin loop for each case. GCC command: `gcc -O3 -mpopcnt -std=c11 -Wall -Wextra -Wpedantic -pthread bench-competitive.c dispatch.o kernels.o -o bench_competitive`.

## Interpretation

- The current AVX2 kernel is materially faster than a simple scalar/builtin POPCNT loop *on these local workloads*.
- This AVX-512 candidate improves small working-set speed locally, but is **not consistently fastest** on larger data and sparse inputs; input size and cache footprint matter.
- Different compiler flags, physical CPUs, microarchitectures, simultaneous workload and memory systems can reverse rankings. One cloud CPU cannot establish a SOTA claim.
- The repo's current dispatch picks AVX2 when present even if the CPU supports AVX-512 VPOPCNTDQ. This is a plausible, testable extension point, not automatically a bug.
- Original scalar Kernighan loop is an intentionally weak performance baseline for dense inputs, and should NOT be the flagship performance comparator.

## BF-02 recommended execution

1. **Baseline:** preserve working reference, compile GCC/Clang native optimized C (`__builtin_popcountll`), current AVX2, AVX-512 (when supported), and strong third-party baselines where semantically comparable.
2. **Correctness:** exhaustive lengths 0–128, randomized lengths with adversarial tails/aliasing, guard pages, saturation & long arrays, ASan/UBSan where possible; VPOPCNTDQ runtime gating and ABI validation.
3. **Benchmark:** 64 B to >=128 MiB total input, dense/sparse/clustered, aligned and shifted word alignments, warm and cold cache, pinned core, compiler provenance and repeated median/p95; use `perf stat` cycles/instructions/cache-misses only when environment allows reliable measurements.
4. **One variable:** experimentally compare AVX2 vs AVX-512 vs optimized C and propose a size/ISA dispatch threshold; measure dispatch overhead for tiny inputs.
5. **Go/no-go:** promote only if correctness tests all pass and the performance improvement holds across independent machines/workloads without material regressions. Record failures and negative results.

## Reproduce locally

```
cd bitforge-audit

gcc -O3 -std=c11 -Wall -Wextra -Wpedantic -c dispatch.c
gcc -c kernels.S
gcc -O3 -pthread test.c dispatch.o kernels.o -o bitforge_test
./bitforge_test

gcc -O3 -mpopcnt -std=c11 -Wall -Wextra -Wpedantic -pthread bench-competitive.c dispatch.o kernels.o -o bench_competitive
taskset -c 0 ./bench_competitive
```

The benchmark invokes the AVX-512 variant only on CPUs with `avx512vpopcntdq`; it invokes the original ASM variants only where corresponding ISA flags are available. The target attribute keeps AVX-512 instructions in the candidate function. The assembly reference code is Linux/System V AMD64-specific.

## Claims not made

- Not a whole-program performance benchmark, database benchmark, Roaring compressed-bitmap comparison, public leaderboard result, hardware-independent speedup, AVX-512 production implementation, fuzzed proof, SOTA result, or upstream GitHub change.
