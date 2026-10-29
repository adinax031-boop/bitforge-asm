#define _POSIX_C_SOURCE 200809L
#include "bitforge.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
static double now(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static void run(const char *name,uint64_t (*fn)(const uint64_t*,const uint64_t*,size_t),const uint64_t*a,const uint64_t*b,size_t n,size_t reps){
 volatile uint64_t sink=0; double start=now(); for(size_t i=0;i<reps;i++) sink += fn(a,b,n); double elapsed=now()-start;
 printf("%-8s %9.3f ns/word  %8.2f GiB/s  checksum=%llu\n",name,elapsed*1e9/(n*reps),(double)n*reps*16/elapsed/1073741824.,(unsigned long long)sink);
}
int main(void){size_t n=1<<20,reps=100; uint64_t *a=malloc(n*8),*b=malloc(n*8); if(!a||!b)return 1;
 uint64_t x=42;for(size_t i=0;i<n;i++){x^=x<<13;x^=x>>7;x^=x<<17;a[i]=x;x^=x<<13;x^=x>>7;x^=x<<17;b[i]=x;}
 printf("words=%zu repetitions=%zu dispatch=%s; two input streams\n",n,reps,bf_backend());
 run("scalar",bf_intersect_count_scalar,a,b,n,reps);if(__builtin_cpu_supports("popcnt"))run("popcnt",bf_intersect_count_popcnt,a,b,n,reps);if(__builtin_cpu_supports("avx2"))run("avx2",bf_intersect_count_avx2,a,b,n,reps);run("dispatch",bf_intersect_count,a,b,n,reps);free(a);free(b);}
