/*
* Find min/max element value in array
* with SIMD(AVX)
* Probably, working on C
* 
* ref
* https://learn.microsoft.com/en-us/cpp/intrinsics/x86-intrinsics-list?view=msvc-170
* https://learn.microsoft.com/en-us/cpp/intrinsics/x64-amd64-intrinsics-list?view=msvc-170
*/


// Manual SIMD replacement loop, compiler auto-vectorization disabled.
#pragma clang loop vectorize(disable)

#include "MinMaxSIMD.h"

#include <limits.h>
#include <float.h>
#include <immintrin.h>


/*
* Find min element value in int16 array.
*/
extern "C" int16_t find_min_i16(const int16_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT16_MAX;

    size_t idx = 0;
    int16_t min_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int16_t);  // 16

    if (len >= batch)
    {
        __m256i minv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            minv = _mm256_min_epi16(minv, v);
        }

        alignas(32) int16_t temp[batch];  // AVX2 -> 32B register width
        _mm256_store_si256((__m256i*)temp, minv);

        // Find in last register
        min_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            min_val = (temp[temp_idx] < min_val) ? temp[temp_idx] : min_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        min_val = (array[idx] < min_val) ? array[idx] : min_val;
    }

    _mm256_zeroupper();
    return min_val;
}

/*
* Find max element value in int16 array.
*/
extern "C" int16_t find_max_i16(const int16_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT16_MIN;

    size_t idx = 0;
    int16_t max_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int16_t);  // 16

    if (len >= batch)
    {
        __m256i maxv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            maxv = _mm256_max_epi16(maxv, v);
        }

        alignas(32) int16_t temp[batch];  // AVX2 -> 32B register width
        _mm256_store_si256((__m256i*)temp, maxv);

        // Find in last register
        max_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            max_val = (temp[temp_idx] > max_val) ? temp[temp_idx] : max_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        max_val = (array[idx] > max_val) ? array[idx] : max_val;
    }

    _mm256_zeroupper();
    return max_val;
}

/*
* Find min element value in int32 array.
*/
extern "C" int32_t find_min_i32(const int32_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT32_MAX;

    size_t idx = 0;
    int32_t min_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int32_t);  // 8

    if (len >= batch)
    {
        __m256i minv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            minv = _mm256_min_epi32(minv, v);
        }

        alignas(32) int32_t temp[batch];  // AVX -> 32B register width
        _mm256_store_si256((__m256i*)temp, minv);

        // Find in last register
        min_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            min_val = (temp[temp_idx] < min_val) ? temp[temp_idx] : min_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        min_val = (array[idx] < min_val) ? array[idx] : min_val;
    }

    _mm256_zeroupper();
    return min_val;
}

/*
* Find max element value in int32 array.
*/
extern "C" int32_t find_max_i32(const int32_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT32_MIN;

    size_t idx = 0;
    int32_t max_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int32_t);  // 8

    if (len >= batch)
    {
        __m256i maxv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            maxv = _mm256_max_epi32(maxv, v);
        }

        alignas(32) int32_t temp[batch];  // AVX2 -> 32B register width
        _mm256_store_si256((__m256i*)temp, maxv);

        // Find in last register
        max_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            max_val = (temp[temp_idx] > max_val) ? temp[temp_idx] : max_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        max_val = (array[idx] > max_val) ? array[idx] : max_val;
    }

    _mm256_zeroupper();
    return max_val;
}

/*
* Find min element value in int64 array.
*/
extern "C" int64_t find_min_i64(const int64_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT64_MAX;

    size_t idx = 0;
    int64_t min_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int64_t);  // 4

    if (len >= batch)
    {
        __m256i minv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            __m256i mask = _mm256_cmpgt_epi64(v, minv);
            minv = _mm256_blendv_epi8(v, minv, mask);
        }

        alignas(32) int64_t temp[batch];  // AVX2 -> 32B register width
        _mm256_store_si256((__m256i*)temp, minv);

        // Find in last register
        min_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            min_val = (temp[temp_idx] < min_val) ? temp[temp_idx] : min_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        min_val = (array[idx] < min_val) ? array[idx] : min_val;
    }

    _mm256_zeroupper();
    return min_val;
}

/*
* Find max element value in int64 array.
*/
extern "C" int64_t find_max_i64(const int64_t* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return INT64_MIN;

    size_t idx = 0;
    int64_t max_val = array[0];
    const size_t batch = sizeof(__m256i) / sizeof(int64_t);  // 4

    if (len >= batch)
    {
        __m256i maxv = _mm256_loadu_si256((const __m256i*)array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256i v = _mm256_loadu_si256((const __m256i*)(array + idx));
            __m256i mask = _mm256_cmpgt_epi64(maxv, v);
            maxv = _mm256_blendv_epi8(v, maxv, mask);
        }

        alignas(32) int64_t temp[batch];  // AVX2 -> 32B register width
        _mm256_store_si256((__m256i*)temp, maxv);

        // Find in last register
        max_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            max_val = (temp[temp_idx] > max_val) ? temp[temp_idx] : max_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        max_val = (array[idx] > max_val) ? array[idx] : max_val;
    }

    _mm256_zeroupper();
    return max_val;
}

/*
* Find min element value in float array.
*/
extern "C" float find_min_f32(const float* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return FLT_MAX;

    size_t idx = 0;
    float min_val = array[0];
    const size_t batch = sizeof(__m256) / sizeof(float);  // 8

    if (len >= batch)
    {
        __m256 minv = _mm256_loadu_ps(array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256 v = _mm256_loadu_ps(array + idx);
            minv = _mm256_min_ps(minv, v);
        }

        alignas(32) float temp[batch];  // AVX2 -> 32B register width
        _mm256_store_ps(temp, minv);

        // Find in last register
        min_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            min_val = (temp[temp_idx] < min_val) ? temp[temp_idx] : min_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        min_val = (array[idx] < min_val) ? array[idx] : min_val;
    }

    _mm256_zeroupper();
    return min_val;
}

/*
* Find max element value in float array.
*/
extern "C" float find_max_f32(const float* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return -FLT_MAX;

    size_t idx = 0;
    float max_val = array[0];
    const size_t batch = sizeof(__m256) / sizeof(float);  // 8

    if (len >= batch)
    {
        __m256 maxv = _mm256_loadu_ps(array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256 v = _mm256_loadu_ps(array + idx);
            maxv = _mm256_max_ps(maxv, v);
        }

        alignas(32) float temp[batch];  // AVX2 -> 32B register width
        _mm256_store_ps(temp, maxv);

        // Find in last register
        max_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            max_val = (temp[temp_idx] > max_val) ? temp[temp_idx] : max_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        max_val = (array[idx] > max_val) ? array[idx] : max_val;
    }

    _mm256_zeroupper();
    return max_val;
}

/*
* Find min element value in double array.
*/
extern "C" double find_min_f64(const double* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return DBL_MAX;

    size_t idx = 0;
    double min_val = array[0];
    const size_t batch = sizeof(__m256d) / sizeof(double);  // 4

    if (len >= batch)
    {
        __m256d minv = _mm256_loadu_pd(array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256d v = _mm256_loadu_pd(array + idx);
            minv = _mm256_min_pd(minv, v);
        }

        alignas(32) double temp[batch];  // AVX2 -> 32B register width
        _mm256_store_pd(temp, minv);

        // Find in last register
        min_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            min_val = (temp[temp_idx] < min_val) ? temp[temp_idx] : min_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        min_val = (array[idx] < min_val) ? array[idx] : min_val;
    }

    _mm256_zeroupper();
    return min_val;
}

/*
* Find max element value in double array.
*/
extern "C" double find_max_f64(const double* RESTRICT array, const size_t len)
{
    if (!array || len == 0) return -DBL_MAX;

    size_t idx = 0;
    double max_val = array[0];
    const size_t batch = sizeof(__m256d) / sizeof(double);  // 4

    if (len >= batch)
    {
        __m256d maxv = _mm256_loadu_pd(array);
        idx += batch;

        // Find in registers
        for (; idx + batch <= len; idx += batch)
        {
            __m256d v = _mm256_loadu_pd(array + idx);
            maxv = _mm256_max_pd(maxv, v);
        }

        alignas(32) double temp[batch];  // AVX2 -> 32B register width
        _mm256_store_pd(temp, maxv);

        // Find in last register
        max_val = temp[0];
        for (size_t temp_idx = 1; temp_idx < batch; ++temp_idx)
        {
            max_val = (temp[temp_idx] > max_val) ? temp[temp_idx] : max_val;
        }
    }

    // Find in remain elements
    for (; idx < len; ++idx)
    {
        max_val = (array[idx] > max_val) ? array[idx] : max_val;
    }

    _mm256_zeroupper();
    return max_val;
}