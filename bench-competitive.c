#define _POSIX_C_SOURCE 200809L
#include "bitforge.h"
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
static uint64_t rng=0x1deccfa24387593ULL;
static uint64_t random64(void){rng ^= rng>>12; rng ^=rng<<25; rng^=rng>>27; return rng*2685821657736338717ULL;}
static double sec(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (double)t.tv_sec+(double)t.tv_nsec/1e9;}
__attribute__((noinline)) uint64_t builtin_popcnt(const uint64_t*a,const uint64_t*b,size_t n){uint64_t s=0;for(size_t i=0;i<n;i++)s+=(uint64_t)__builtin_popcountll(a[i]&b[i]);return s;}
__attribute__((noinline,target("avx512f,avx512vpopcntdq")))
uint64_t avx512_popcnt(const uint64_t*a,const uint64_t*b,size_t n){
  __m512i sum=_mm512_setzero_si512();size_t i=0;
  for(;i+8<=n;i+=8){__m512i x=_mm512_loadu_si512((const void*)(a+i));__m512i y=_mm512_loadu_si512((const void*)(b+i));sum=_mm512_add_epi64(sum,_mm512_popcnt_epi64(_mm512_and_si512(x,y)));}
  uint64_t lanes[8];_mm512_storeu_si512((void*)lanes,sum);uint64_t s=0;for(int j=0;j<8;j++)s+=lanes[j];for(;i<n;i++)s+=(uint64_t)__builtin_popcountll(a[i]&b[i]);return s;
}
static int cmpd(const void *aa,const void *bb){double a=*(const double*)aa,b=*(const double*)bb;return (a>b)-(a<b);}
static volatile uint64_t sink;
static double measure(uint64_t(*fn)(const uint64_t*,const uint64_t*,size_t), const uint64_t*a,const uint64_t*b,size_t n,size_t reps){
  double m[7];for(int trial=0;trial<7;trial++){double start=sec();uint64_t h=0;for(size_t r=0;r<reps;r++)h+=fn(a,b,n);double elapsed=sec()-start;sink^=h;m[trial]=elapsed/(double)(n*reps)*1e9;}qsort(m,7,sizeof(double),cmpd);return m[3];
}
int main(void){printf("gcc competitive microbenchmark, dispatch=%s\n",bf_backend());
  size_t sizes[]={8192, 1<<20}; const char* names[]={"64KiB/stream","8MiB/stream"};
  uint64_t (*fn[])(const uint64_t*,const uint64_t*,size_t)={bf_intersect_count_scalar,bf_intersect_count_popcnt,bf_intersect_count_avx2,builtin_popcnt,avx512_popcnt,bf_intersect_count};
  const char* modes[]={"Kernighan","asm_popcnt","asm_avx2","gcc_builtin","AVX512_vpop","dispatch"};
  int avx512=__builtin_cpu_supports("avx512vpopcntdq");
  for(int sz=0;sz<2;sz++){size_t n=sizes[sz];uint64_t*a=malloc(n*8),*b=malloc(n*8);if(!a||!b)return 2;
    size_t reps=(1<<23)/n;if(reps<8)reps=8;
    for(int density=0;density<2;density++){for(size_t i=0;i<n;i++){uint64_t x=random64(),y=random64();if(density){x&=random64() &random64()&random64()&random64();y&=random64()&random64()&random64()&random64();}a[i]=x;b[i]=y;}
      uint64_t expected=builtin_popcnt(a,b,n); printf("%s density=%s n=%zu repeats=%zu expected=%" PRIu64 "\n",names[sz],density?"sparse":"dense",n,reps,expected);
      for(int j=0;j<6;j++){if(j==4&&!avx512)continue;if(j==2&&!__builtin_cpu_supports("avx2"))continue;if(j==1&&!__builtin_cpu_supports("popcnt"))continue;
        if(fn[j](a,b,n)!=expected){printf("MISMATCH %s\n",modes[j]);return 3;}double val=measure(fn[j],a,b,n,reps);printf("  %-13s %.4f ns/word\n",modes[j],val);}
    }free(a);free(b);}
  printf("sink=%" PRIu64 "\n",sink);return 0;
}
