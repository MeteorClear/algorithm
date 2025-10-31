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

#ifdef __cplusplus
extern "C" {
#endif

int16_t find_min_i16(const int16_t* RESTRICT array, const size_t len);
int16_t find_max_i16(const int16_t* RESTRICT array, const size_t len);

int32_t find_min_i32(const int32_t* RESTRICT array, const size_t len);
int32_t find_max_i32(const int32_t* RESTRICT array, const size_t len);

int64_t find_min_i64(const int64_t* RESTRICT array, const size_t len);	// Technical implementation, slow and heavy
int64_t find_max_i64(const int64_t* RESTRICT array, const size_t len);  // Technical implementation, slow and heavy

float find_min_f32(const float* RESTRICT array, const size_t len);
float find_max_f32(const float* RESTRICT array, const size_t len);

double find_min_f64(const double* RESTRICT array, const size_t len);
double find_max_f64(const double* RESTRICT array, const size_t len);

#ifdef __cplusplus
}
#endif