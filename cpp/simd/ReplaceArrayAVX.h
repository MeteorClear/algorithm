#pragma once

#include <stdint.h>
#include <stddef.h>

#if defined(__cplusplus)
#  if defined(_MSC_VER)
#    define RESTRICT __restrict
#  else
#    define RESTRICT __restrict__
#  endif
#else
#  define RESTRICT restrict
#endif

// For C interop
#ifdef __cplusplus
extern "C" {
#endif

int replace_char_avx(char* RESTRICT array, const size_t len, const char target, const char dest);

int replace_i16_avx(int16_t* RESTRICT array, const size_t len, const int16_t target, const int16_t dest);
int replace_i32_avx(int32_t* RESTRICT array, const size_t len, const int32_t target, const int32_t dest);
int replace_i64_avx(int64_t* RESTRICT array, const size_t len, const int64_t target, const int64_t dest);

int replace_float_avx(float* RESTRICT array, const size_t len, const float target, const float dest);
int replace_float_epsilon_avx(float* RESTRICT array, const size_t len, const float target, const float dest, const float eps);
int replace_double_avx(double* RESTRICT array, const size_t len, const double target, const double dest);
int replace_double_epsilon_avx(double* RESTRICT array, size_t len, const double target, const double dest, const double eps);

#ifdef __cplusplus
}
#endif
