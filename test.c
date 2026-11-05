#define _GNU_SOURCE
#include "bitforge.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
static uint64_t state=0x123456789abcdefULL;
static uint64_t rnd(void) { state^=state<<13; state^=state>>7; state^=state<<17; return state; }
static void check(const uint64_t *a,const uint64_t *b,size_t n) {
    uint64_t expected=0;
    for (size_t i=0;i<n;i++) expected += (uint64_t)__builtin_popcountll(a[i]&b[i]);
    assert(bf_intersect_count_scalar(a,b,n)==expected);
    assert(bf_intersect_count(a,b,n)==expected);
    if (__builtin_cpu_supports("popcnt")) assert(bf_intersect_count_popcnt(a,b,n)==expected);
    if (__builtin_cpu_supports("avx2")) assert(bf_intersect_count_avx2(a,b,n)==expected);
}
static void *thread(void *p) { (void)p; for(int i=0;i<1000;i++) assert(bf_intersect_count(NULL,NULL,0)==0); return NULL; }
int main(void) {
    pthread_t ts[8]; for(int i=0;i<8;i++) assert(!pthread_create(&ts[i],NULL,thread,NULL));
    for(int i=0;i<8;i++) assert(!pthread_join(ts[i],NULL));
    check(NULL,NULL,0);
    uint64_t a[520],b[520];
    for(int round=0;round<100;round++) {
        for(size_t i=0;i<520;i++){a[i]=rnd();b[i]=rnd();}
        for(size_t n=0;n<260;n++) {check(a+1,b+2,n);check(a+1,a+1,n);}
    }
    memset(a,255,sizeof a);memset(b,255,sizeof b);
    for(size_t n=0;n<520;n++) check(a,b,n);
    for (size_t bit=0;bit<64;bit++) {
        for (size_t i=0;i<520;i++) { a[i]=UINT64_C(1)<<bit; b[i]=(i&1)?a[i]:~a[i]; }
        for (size_t n=0;n<=128;n++) check(a,b,n);
    }
    /* All pairs of bit positions, including non-intersecting single bits. */
    for (size_t i=0;i<64;i++) for (size_t j=0;j<64;j++) {
        uint64_t x=UINT64_C(1)<<i, y=UINT64_C(1)<<j;
        check(&x,&y,1);
    }
    size_t page=(size_t)sysconf(_SC_PAGESIZE);
    char *m=mmap(NULL,page*4,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(m!=MAP_FAILED); assert(!mprotect(m+page,page,PROT_NONE)); assert(!mprotect(m+3*page,page,PROT_NONE));
    for(size_t n=0;n<=64;n++) { uint64_t *x=(uint64_t *)(m+page-n*8), *y=(uint64_t *)(m+3*page-n*8); for(size_t i=0;i<n;i++){x[i]=rnd();y[i]=rnd();} check(x,y,n); }
    munmap(m,page*4);
    /* A leading guard catches underreads independently of the end guard. */
    m=mmap(NULL,page*4,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(m!=MAP_FAILED); assert(!mprotect(m,page,PROT_NONE)); assert(!mprotect(m+2*page,page,PROT_NONE));
    for(size_t n=0;n<=64;n++) { uint64_t *x=(uint64_t *)(m+page), *y=(uint64_t *)(m+3*page); for(size_t i=0;i<n;i++){x[i]=rnd();y[i]=rnd();} check(x,y,n); }
    assert(!munmap(m,page*4));
    printf("PASS: randomized differential, tails, aliasing, guard pages, concurrent initialization; dispatch=%s\n",bf_backend());
}
