#pragma once

// For C interop
#ifdef __cplusplus
extern "C" {
#endif

// charactor
void replace_char_sse(char* array, size_t len, char target, char dest);

// integer
void replace_short_sse(short* array, size_t len, short target, short dest);
void replace_int_sse(int* array, size_t len, int target, int dest);
void replace_longlong_sse(long long* array, size_t len, long long target, long long dest);

// floating point
void replace_float_sse(float* array, size_t len, float target, float dest);
void replace_float_epsilon_sse(float* array, size_t len, float target, float dest, float eps);
void replace_double_sse(double* array, size_t len, double target, double dest);
void replace_double_epsilon_sse(double* array, size_t len, double target, double dest, double eps);

#ifdef __cplusplus
}
#endif
