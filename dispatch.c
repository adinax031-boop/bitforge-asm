#include "bitforge.h"
#include <pthread.h>
static uint64_t (*selected)(const uint64_t *, const uint64_t *, size_t);
static const char *name;
uint64_t bf_intersect_count_scalar(const uint64_t *a, const uint64_t *b, size_t n) {
    uint64_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        uint64_t x = a[i] & b[i];
        while (x) { x &= x - 1; ++sum; }
    }
    return sum;
}
static void init(void) {
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) { selected = bf_intersect_count_avx2; name = "avx2"; }
    else if (__builtin_cpu_supports("popcnt")) { selected = bf_intersect_count_popcnt; name = "popcnt"; }
    else { selected = bf_intersect_count_scalar; name = "scalar"; }
}
static pthread_once_t once = PTHREAD_ONCE_INIT;
uint64_t bf_intersect_count(const uint64_t *a, const uint64_t *b, size_t n) {
    pthread_once(&once, init); return selected(a,b,n);
}
const char *bf_backend(void) { pthread_once(&once, init); return name; }
