#pragma once

// For C interop
#ifdef __cplusplus
extern "C" {
#endif

int replace_char_avx(char* array, size_t len, char target, char dest);

int replace_short_avx(short* array, size_t len, short target, short dest);
int replace_int_avx(int* array, size_t len, int target, int dest);
int replace_longlong_avx(long long* array, size_t len, long long target, long long dest);

int replace_float_avx(float* array, size_t len, float target, float dest);
int replace_float_epsilon_avx(float* array, size_t len, float target, float dest, float eps);
int replace_double_avx(double* array, size_t len, double target, double dest);
int replace_double_epsilon_avx(double* array, size_t len, double target, double dest, double eps);

#ifdef __cplusplus
}
#endif
