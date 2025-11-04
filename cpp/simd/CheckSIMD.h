/*
* Check SIMD compatibility.
* SIMD(Single Instruction stream, Multiple Data streams)
* Can check SSE, XOP, FMA, AVX, AVX512.
* Probably, working on C
*/
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t check_SSE();
uint32_t check_AMD();
uint32_t check_AVX();
uint32_t check_AVX512();
uint64_t check_SIMD();

#ifdef __cplusplus
}
#endif