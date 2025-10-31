/*
* Check SIMD compatibility.
* SIMD(Single Instruction stream, Multiple Data streams)
* Can check SSE, XOP, FMA, AVX, AVX512.
* Probably, working on C
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int check_SSE_level();
int check_XOP_level();
int check_FMA_level();
int check_AVX_level();
unsigned int check_AVX512();

void init_SIMD_LEVEL();

#ifdef __cplusplus
}
#endif