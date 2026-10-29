/* System V AMD64 ABI. GNU assembler, Intel syntax. No callee-saved registers. */
.intel_syntax noprefix
.text
.globl bf_intersect_count_popcnt
.type bf_intersect_count_popcnt,@function
bf_intersect_count_popcnt:
    xor eax, eax
    xor ecx, ecx
.Lscalar:
    cmp rcx, rdx
    jae .Lscalar_done
    mov r8, [rdi+rcx*8]
    and r8, [rsi+rcx*8]
    popcnt r8, r8
    add rax, r8
    inc rcx
    jmp .Lscalar
.Lscalar_done:
    ret
.size bf_intersect_count_popcnt, .-bf_intersect_count_popcnt

/* AVX2 nibble LUT: vpshufb independently looks up both 128-bit lanes.
   vpsadbw widens each group of eight byte counts before summing, avoiding
   overflow in byte accumulators. Scalar tail uses a software bit count;
   AVX2 does not architecturally imply POPCNT. */
.globl bf_intersect_count_avx2
.type bf_intersect_count_avx2,@function
bf_intersect_count_avx2:
    xor eax, eax
    xor ecx, ecx
    vpxor ymm4, ymm4, ymm4
    vmovdqu ymm5, YMMWORD PTR [rip+.Llut]
    vmovdqu ymm6, YMMWORD PTR [rip+.Lmask]
.Lvector:
    mov r8, rdx
    sub r8, rcx
    cmp r8, 4
    jb .Lreduce
    vmovdqu ymm0, YMMWORD PTR [rdi+rcx*8]
    vpand ymm0, ymm0, YMMWORD PTR [rsi+rcx*8]
    vpand ymm1, ymm0, ymm6
    vpsrlw ymm0, ymm0, 4
    vpand ymm0, ymm0, ymm6
    vpshufb ymm1, ymm5, ymm1
    vpshufb ymm0, ymm5, ymm0
    vpaddb ymm0, ymm0, ymm1
    vpxor ymm1, ymm1, ymm1
    vpsadbw ymm0, ymm0, ymm1
    vpaddq ymm4, ymm4, ymm0
    add rcx, 4
    jmp .Lvector
.Lreduce:
    vextracti128 xmm0, ymm4, 1
    vpaddq xmm0, xmm0, xmm4
    vpsrldq xmm1, xmm0, 8
    vpaddq xmm0, xmm0, xmm1
    vmovq rax, xmm0
    vzeroupper
.Ltail:
    cmp rcx, rdx
    jae .Ldone
    mov r8, [rdi+rcx*8]
    and r8, [rsi+rcx*8]
.Lbits:
    test r8, r8
    jz .Lnext
    lea r9, [r8-1]
    and r8, r9
    inc rax
    jmp .Lbits
.Lnext:
    inc rcx
    jmp .Ltail
.Ldone:
    ret
.size bf_intersect_count_avx2, .-bf_intersect_count_avx2
.section .rodata
.p2align 5
.Llut:
.byte 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4
.byte 0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4
.Lmask:
.fill 32,1,15
.section .note.GNU-stack,"",@progbits
