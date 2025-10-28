/*
* Replacing elements of array
* with SSE(Streaming SIMD(Single Instruction, Multiple Data) Extensions)
* Probably, working on C
* 
* At least, 2-3 times faster than std::replace and for-iteration.
* Small array size means less difference
* 
* ref
* https://learn.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-170
* https://learn.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list?view=msvc-170
*/


// Manual SIMD replacement loop, compiler auto-vectorization disabled.
#pragma clang loop vectorize(disable)


#include "ReplaceArraySSE.h"
#include <math.h>
#include <intrin.h>
#include <emmintrin.h>  // SSE2
#include <smmintrin.h>  // SSE4.1


static int SSE4_1 = -1;  // SSE4.1 cache, -1 = unknown, 0 = unsupport, 1 = support
static int check_SSE4_1()
{
    if (SSE4_1 != -1) return SSE4_1;

    int cpu_info[4]; // EAX, EBX, ECX, EDX

#ifdef _MSC_VER
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a" (cpu_info[0]), "=b" (cpu_info[1]), "=c" (cpu_info[2]), "=d" (cpu_info[3])
        : "a" (1)
    );
#endif

    // Check bit 19 of ECX for SSE 4.1 support
    SSE4_1 = (cpu_info[2] & (1 << 19)) ? 1 : 0;
    return SSE4_1;
}


/*
* Replace target characters to destination characters from string.
*/
extern "C" void replace_char_sse(char* array, size_t len, char target, char dest)
{
    if (!array || len <= 0) return;
    if (target == dest) return;

    __m128i target_ = _mm_set1_epi8(target);
    __m128i dest_   = _mm_set1_epi8(dest);

    const size_t batch = sizeof(__m128i) / sizeof(char);  // 16
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128i data = _mm_loadu_si128((__m128i*)(array + i));
        __m128i mask = _mm_cmpeq_epi8(data, target_);
        __m128i blend = has_sse4_1
            ? _mm_blendv_epi8(data, dest_, mask)
            : _mm_or_si128(_mm_and_si128(mask, dest_), _mm_andnot_si128(mask, data));
        _mm_storeu_si128((__m128i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target short integers to destination short integers from short array.
*/
extern "C" void replace_short_sse(short* array, size_t len, short target, short dest)
{
    if (!array || len == 0) return;
    if (target == dest) return;

    __m128i target_ = _mm_set1_epi16(target);
    __m128i dest_   = _mm_set1_epi16(dest);

    const size_t batch = sizeof(__m128i) / sizeof(short);  // 8
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128i data  = _mm_loadu_si128((__m128i*)(array + i));
        __m128i mask  = _mm_cmpeq_epi16(data, target_);
        __m128i blend = has_sse4_1
            ? _mm_blendv_epi8(data, dest_, mask)
            : _mm_or_si128(_mm_and_si128(mask, dest_), _mm_andnot_si128(mask, data));
        _mm_storeu_si128((__m128i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target integers to destination integers from int array.
*/
extern "C" void replace_int_sse(int* array, size_t len, int target, int dest)
{
    if (!array || len == 0) return;
    if (target == dest) return;

    __m128i target_ = _mm_set1_epi32(target);
    __m128i dest_   = _mm_set1_epi32(dest);

    const size_t batch = sizeof(__m128i) / sizeof(int);  // 4
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128i data  = _mm_loadu_si128((__m128i*)(array + i));
        __m128i mask  = _mm_cmpeq_epi32(data, target_);
        __m128i blend = has_sse4_1
            ? _mm_blendv_epi8(data, dest_, mask)
            : _mm_or_si128(_mm_and_si128(mask, dest_), _mm_andnot_si128(mask, data));
        _mm_storeu_si128((__m128i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target 64-bit integers to destination 64-bit integers from long long array.
*/
extern "C" void replace_longlong_sse(long long* array, size_t len, long long target, long long dest)
{
    if (!array || len == 0) return;
    if (target == dest) return;
    if (!check_SSE4_1()) return;  // SSE4.1 work only

    __m128i target_ = _mm_set1_epi64x(target);
    __m128i dest_ = _mm_set1_epi64x(dest);

    const size_t batch = sizeof(__m128i) / sizeof(long long);  // 2

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128i data  = _mm_loadu_si128((__m128i*)(array + i));
        __m128i mask  = _mm_cmpeq_epi64(data, target_);
        __m128i blend = _mm_blendv_epi8(data, dest_, mask);
        _mm_storeu_si128((__m128i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target floats to destination floats from float array.
*/
extern "C" void replace_float_sse(float* array, size_t len, float target, float dest)
{
    if (!array || len == 0) return;
    if (target == dest) return;

    const __m128 target_ = _mm_set1_ps(target);
    const __m128 dest_   = _mm_set1_ps(dest);

    const size_t batch = sizeof(__m128) / sizeof(float);  // 4
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128 data  = _mm_loadu_ps(array + i);
        __m128 mask  = _mm_cmpeq_ps(data, target_);
        __m128 blend = has_sse4_1
            ? _mm_blendv_ps(data, dest_, mask)
            : _mm_or_ps(_mm_and_ps(mask, dest_), _mm_andnot_ps(mask, data));
        _mm_storeu_ps(array + i, blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target floats to destination floats from float array.
* Epsilon range version.
*/
extern "C" void replace_float_epsilon_sse(
    float* array, size_t len, float target, float dest, float eps)
{
    if (!array || len == 0) return;

    const __m128 target_ = _mm_set1_ps(target);
    const __m128 dest_   = _mm_set1_ps(dest);
    const __m128 eps_    = _mm_set1_ps(eps);

    const size_t batch = sizeof(__m128) / sizeof(float);  // 4
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128 data  = _mm_loadu_ps(array + i);
        __m128 diff  = _mm_andnot_ps(_mm_set1_ps(-0.0f), _mm_sub_ps(data, target_));  // |data - target|, remove sign bit
        __m128 mask  = _mm_cmple_ps(diff, eps_);  // diff <= eps, mask = true
        __m128 blend = has_sse4_1
            ? _mm_blendv_ps(data, dest_, mask)
            : _mm_or_ps(_mm_and_ps(mask, dest_), _mm_andnot_ps(mask, data));
        _mm_storeu_ps(array + i, blend);
    }

    for (; i < len; ++i)
    {
        float diff = fabsf(array[i] - target);
        if (diff <= eps) array[i] = dest;
    }
}

/*
* Replace target doubles to destination doubles from double array.
*/
extern "C" void replace_double_sse(double* array, size_t len, double target, double dest)
{
    if (!array || len == 0) return;
    if (target == dest) return;

    const __m128d target_ = _mm_set1_pd(target);
    const __m128d dest_   = _mm_set1_pd(dest);

    const size_t batch = sizeof(__m128d) / sizeof(double);  // 2
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128d data  = _mm_loadu_pd(array + i);
        __m128d mask  = _mm_cmpeq_pd(data, target_);
        __m128d blend = has_sse4_1
            ? _mm_blendv_pd(data, dest_, mask)
            : _mm_or_pd(_mm_and_pd(mask, dest_), _mm_andnot_pd(mask, data));
        _mm_storeu_pd(array + i, blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }
}

/*
* Replace target doubles to destination doubles from double array.
* Epsilon range version.
*/
extern "C" void replace_double_epsilon_sse(
    double* array, size_t len, double target, double dest, double eps)
{
    if (!array || len == 0) return;

    const __m128d target_ = _mm_set1_pd(target);
    const __m128d dest_   = _mm_set1_pd(dest);
    const __m128d eps_    = _mm_set1_pd(eps);

    const size_t batch = sizeof(__m128d) / sizeof(double);  // 2
    const int has_sse4_1 = check_SSE4_1();

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m128d data = _mm_loadu_pd(array + i);
        __m128d diff = _mm_andnot_pd(_mm_set1_pd(-0.0), _mm_sub_pd(data, target_));   // |data - target|, remove sign bit
        __m128d mask = _mm_cmple_pd(diff, eps_);  // diff <= eps, mask = true
        __m128d blend = has_sse4_1
            ? _mm_blendv_pd(data, dest_, mask)
            : _mm_or_pd(_mm_and_pd(mask, dest_), _mm_andnot_pd(mask, data));
        _mm_storeu_pd(array + i, blend);
    }

    for (; i < len; ++i)
    {
        double diff = fabs(array[i] - target);
        if (diff <= eps) array[i] = dest;
    }
}

/*
* SSE4.1
* __m128i _mm_blendv_epi8 (__m128i, __m128i, __m128i);
* __m128i _mm_cmpeq_epi64(__m128i, __m128i);
* __m128 _mm_blendv_ps(__m128, __m128, __m128);
* __m128d _mm_blendv_pd(__m128d, __m128d, __m128d);
*/