#ifndef BITFORGE_H
#define BITFORGE_H
#include <stddef.h>
#include <stdint.h>
/* All functions accept word-aligned inputs without requiring SIMD alignment. n is a count of uint64_t words.
   For n == 0, a and b may be NULL. Otherwise they must each address n words.
   Result is modulo 2^64. The buffers may alias. */
uint64_t bf_intersect_count(const uint64_t *a, const uint64_t *b, size_t n);
uint64_t bf_intersect_count_scalar(const uint64_t *a, const uint64_t *b, size_t n);
/* Direct backends require CPU support: POPCNT and AVX2, respectively. */
uint64_t bf_intersect_count_popcnt(const uint64_t *a, const uint64_t *b, size_t n);
uint64_t bf_intersect_count_avx2(const uint64_t *a, const uint64_t *b, size_t n);
const char *bf_backend(void);
#endif
