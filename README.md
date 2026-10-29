# Bitforge

**A small x86-64 assembly engine for bitmap intersection cardinality.** Bitforge counts the set bits in `A & B` without materializing the intersection. It is a focused systems programming portfolio project: hand-written SIMD, an explicit ABI, runtime feature dispatch, boundary tests, and a reproducible benchmark.

## Build and run

Linux x86-64, GCC or a compatible compiler, GNU assembler, make, and pthreads are required.

```sh
make
make test
make bench
```

The build does not require global `-mavx2` or `-mpopcnt`. Runtime dispatch checks CPU and OS support via the compiler's CPU feature detection and chooses AVX2, POPCNT, or portable scalar code. Dispatch is initialized once with `pthread_once`.

## Architecture

- **AVX2:** load 256 bits from each bitmap, AND, split each byte into nibbles, look up nibble populations using `vpshufb`, and reduce byte counts into 64-bit lanes with `vpsadbw`. The last 0–3 words use a software popcount. `vzeroupper` avoids AVX-to-legacy-SSE transition penalties on affected processors.
- **POPCNT:** one 64-bit word per iteration, using the hardware instruction.
- **Scalar:** portable Kernighan bit counting, available even without POPCNT.

The AVX2 loop accumulates widened counts instead of overflowing byte lanes. Both assembly kernels follow the System V AMD64 calling convention and avoid callee-saved registers. They read no bytes outside the requested range. A zero-length call can use null pointers.

## API

```c
#include "bitforge.h"
uint64_t count = bf_intersect_count(a, b, n_words);
```

Inputs must point to `n_words` readable, naturally aligned `uint64_t` objects; they may alias and need no 32-byte SIMD alignment. The result is modulo 2^64. Direct backend entry points are exposed for measurement and require the caller to check the relevant CPU feature; prefer the dispatched function in applications. This project targets Linux / System V x86-64, not Windows ABI or ARM.

## Verification

`make test` compares all supported backends against the portable reference across deterministic random inputs, many lengths and tails, identical buffers, all-ones input, zero length, concurrent first calls, and buffers ending at protected pages. The guard-page cases detect reads beyond the end, while differential tests compare numerical results. These tests are evidence for the covered cases, not a formal proof.

## Benchmark interpretation

`make bench` reports elapsed time per word and effective input GiB/s for a 16 MiB combined input. It uses a fixed seed and monotonic clock. Results depend on CPU, cache, memory bandwidth, compiler, scheduling, and frequency scaling; no performance claim is universal. Pin to a core and repeat independently for serious comparison. The scalar reference is intentionally simple, not an optimized production baseline. Run `lscpu`, record compiler flags, and benchmark other working-set sizes before drawing conclusions.

## Scope and future work

This is a working single-operation kernel, not a general bitmap database or claim of state-of-the-art performance. Potential next steps include multi-accumulator unrolling, AVX-512 VPOPCNTDQ dispatch, more operations, a documented benchmark matrix, and fuzzing against an independently implemented oracle.

MIT licensed. Contributions and reproducible benchmark reports welcome.
