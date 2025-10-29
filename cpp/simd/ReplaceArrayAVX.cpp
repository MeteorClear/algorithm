/*
* Replacing elements of array
* with AVX
* Probably, working on C
*
* At least, 2 times faster than SSE
* Small array size means less difference
*
* ref
* https://learn.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-170
* https://learn.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list?view=msvc-170
*/


// Manual SIMD replacement loop, compiler auto-vectorization disabled.
#pragma clang loop vectorize(disable)


#include "ReplaceArrayAVX.h"

#ifdef _MSC_VER
#   include <intrin.h>
#else
#   include <cpuid.h>
#endif

#include <math.h>  // fabs, fabsf
#include <immintrin.h>  // AVX, AVX2


static int AVX_LEVEL = -1;  // -1 = unknown, 0 = unsupport, 1 = AVX support, 2 = AVX2 support
static int check_AVX_level()
{
    if (AVX_LEVEL != -1) return AVX_LEVEL;

    int cpu_info[4];  // EAX, EBX, ECX, EDX

#ifdef _MSC_VER
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    int has_OSXSAVE = (cpu_info[2] & (1 << 27)) ? 1 : 0;  // ECX bit 27 = OSXSAVE
    int has_AVX     = (cpu_info[2] & (1 << 28)) ? 1 : 0;  // ECX bit 28 = AVX

    // OS support XSAVE/SRESTORE, CPU support AVX command
    if (!(has_OSXSAVE && has_AVX)) {
        AVX_LEVEL = 0;
        return AVX_LEVEL;
    }

    // Check the XGETBV(XCR0) (XCR0 bit 1: XMM, bit 2: YMM)
    unsigned long long xcr0 = 0;
#ifdef _MSC_VER
    xcr0 = _xgetbv(0);
#else
    unsigned int eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    xcr0 = ((unsigned long long)edx << 32) | eax;
#endif

    // OS support XMM/YMM AVX register context
    if ((xcr0 & 0x6) != 0x6) {
        AVX_LEVEL = 0;
        return AVX_LEVEL;
    }

    // Check the AVX2 bit on leaf 7
#ifdef _MSC_VER
    __cpuidex(cpu_info, 7, 0);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(7), "c"(0)
    );
#endif

    // CPU support AVX2 command
    AVX_LEVEL = (cpu_info[1] & (1 << 5)) ? 2 : 1;  // EBX bit 5 = AVX2
    return AVX_LEVEL;
}


/*
* Replace target characters to destination characters from string.
*/
extern "C" int replace_char_avx(char* array, size_t len, char target, char dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() != 2) return 0;

    const __m256i target_ = _mm256_set1_epi8(target);
    const __m256i dest_   = _mm256_set1_epi8(dest);
    const size_t  batch   = sizeof(__m256i) / sizeof(char);  // 32

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256i data  = _mm256_loadu_si256((__m256i*)(array + i));
        __m256i mask  = _mm256_cmpeq_epi8(data, target_);
        __m256i blend = _mm256_blendv_epi8(data, dest_, mask);
        _mm256_storeu_si256((__m256i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target short integers to destination short integers from short array.
*/
extern "C" int replace_short_avx(short* array, size_t len, short target, short dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() != 2) return 0;

    const __m256i target_ = _mm256_set1_epi16(target);
    const __m256i dest_   = _mm256_set1_epi16(dest);
    const size_t  batch   = sizeof(__m256i) / sizeof(short);  // 16

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256i data  = _mm256_loadu_si256((__m256i*)(array + i));
        __m256i mask  = _mm256_cmpeq_epi16(data, target_);
        __m256i blend = _mm256_blendv_epi8(data, dest_, mask);
        _mm256_storeu_si256((__m256i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target integers to destination integers from int array.
*/
extern "C" int replace_int_avx(int* array, size_t len, int target, int dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() != 2) return 0;

    const __m256i target_ = _mm256_set1_epi32(target);
    const __m256i dest_   = _mm256_set1_epi32(dest);
    const size_t  batch   = sizeof(__m256i) / sizeof(int);  // 8

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256i data  = _mm256_loadu_si256((__m256i*)(array + i));
        __m256i mask  = _mm256_cmpeq_epi32(data, target_);
        __m256i blend = _mm256_blendv_epi8(data, dest_, mask);
        _mm256_storeu_si256((__m256i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target 64-bit integers to destination 64-bit integers from long long array.
*/
extern "C" int replace_longlong_avx(long long* array, size_t len, long long target, long long dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() != 2) return 0;

    const __m256i target_ = _mm256_set1_epi64x(target);
    const __m256i dest_   = _mm256_set1_epi64x(dest);
    const size_t  batch   = sizeof(__m256i) / sizeof(long long);  // 4

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256i data  = _mm256_loadu_si256((__m256i*)(array + i));
        __m256i mask  = _mm256_cmpeq_epi64(data, target_);
        __m256i blend = _mm256_blendv_epi8(data, dest_, mask);
        _mm256_storeu_si256((__m256i*)(array + i), blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target floats to destination floats from float array.
*/
extern "C" int replace_float_avx(float* array, size_t len, float target, float dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() < 1) return 0;

    const __m256 target_ = _mm256_set1_ps(target);
    const __m256 dest_   = _mm256_set1_ps(dest);
    const size_t batch   = sizeof(__m256) / sizeof(float);  // 8 

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256 data  = _mm256_loadu_ps(array + i);
        __m256 mask  = _mm256_cmp_ps(data, target_, _CMP_EQ_OQ);
        __m256 blend = _mm256_blendv_ps(data, dest_, mask);
        _mm256_storeu_ps(array + i, blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target floats to destination floats from float array.
* Epsilon range version.
*/
extern "C" int replace_float_epsilon_avx(
    float* array, size_t len, float target, float dest, float eps)
{
    if (!array || len == 0) return 0;
    if (check_AVX_level() < 1) return 0;

    const __m256 target_   = _mm256_set1_ps(target);
    const __m256 dest_     = _mm256_set1_ps(dest);
    const __m256 eps_      = _mm256_set1_ps(eps);
    const __m256 sign_mask = _mm256_set1_ps(-0.0f);
    const size_t batch     = sizeof(__m256) / sizeof(float);  // 8

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256 data  = _mm256_loadu_ps(array + i);
        __m256 diff  = _mm256_andnot_ps(sign_mask, _mm256_sub_ps(data, target_));  // diff = |data - target|
        __m256 mask  = _mm256_cmp_ps(diff, eps_, _CMP_LE_OQ);  // mask = (diff <= eps)
        __m256 blend = _mm256_blendv_ps(data, dest_, mask);
        _mm256_storeu_ps(array + i, blend);
    }

    for (; i < len; ++i)
    {
        float diff = fabsf(array[i] - target);
        if (diff <= eps) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target doubles to destination doubles from double array.
*/
extern "C" int replace_double_avx(double* array, size_t len, double target, double dest)
{
    if (!array || len == 0) return 0;
    if (target == dest) return 0;
    if (check_AVX_level() < 1) return 0;

    const __m256d target_ = _mm256_set1_pd(target);
    const __m256d dest_   = _mm256_set1_pd(dest);
    const size_t  batch   = sizeof(__m256d) / sizeof(double);  // 4

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256d data  = _mm256_loadu_pd(array + i);
        __m256d mask  = _mm256_cmp_pd(data, target_, _CMP_EQ_OQ);
        __m256d blend = _mm256_blendv_pd(data, dest_, mask);
        _mm256_storeu_pd(array + i, blend);
    }

    for (; i < len; ++i)
    {
        if (array[i] == target) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}

/*
* Replace target doubles to destination doubles from double array.
* Epsilon range version.
*/
extern "C" int replace_double_epsilon_avx(
    double* array, size_t len, double target, double dest, double eps)
{
    if (!array || len == 0) return 0;
    if (check_AVX_level() < 1) return 0;

    const __m256d target_   = _mm256_set1_pd(target);
    const __m256d dest_     = _mm256_set1_pd(dest);
    const __m256d eps_      = _mm256_set1_pd(eps);
    const __m256d sign_mask = _mm256_set1_pd(-0.0);
    const size_t  batch     = sizeof(__m256d) / sizeof(double);  // 4

    size_t i = 0;
    for (; i + batch <= len; i += batch)
    {
        __m256d data  = _mm256_loadu_pd(array + i);
        __m256d diff  = _mm256_andnot_pd(sign_mask, _mm256_sub_pd(data, target_));  // diff = |data - target|
        __m256d mask  = _mm256_cmp_pd(diff, eps_, _CMP_LE_OQ);  // mask = (diff <= eps)
        __m256d blend = _mm256_blendv_pd(data, dest_, mask);
        _mm256_storeu_pd(array + i, blend);
    }

    for (; i < len; ++i)
    {
        double diff = fabs(array[i] - target);
        if (diff <= eps) array[i] = dest;
    }

    _mm256_zeroupper();
    return 1;
}