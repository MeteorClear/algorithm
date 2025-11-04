#include <immintrin.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>


static inline void swap(int32_t* a, int32_t* b) {
    int32_t t = *a;
    *a = *b;
    *b = t;
}

static inline size_t get_next_power_of_two(size_t n) {
    if (n <= 1) return 1;

    if ((n & (n - 1)) == 0) {
        return n;
    }

    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFFUL
    n |= n >> 32;
#endif
    n++;

    return n;
}

static inline void compare_and_swap_avx(int32_t* arr, size_t k, size_t len, int dir)
{
    for (size_t i = 0; i < len; i += 2 * k)
    {
        for (size_t j = i; j + 8 <= i + k; j += 8)
        {
            __m256i va = _mm256_load_si256((__m256i*)(arr + j));
            __m256i vb = _mm256_load_si256((__m256i*)(arr + j + k));
            __m256i minv = _mm256_min_epi32(va, vb);
            __m256i maxv = _mm256_max_epi32(va, vb);
            if (dir) {
                _mm256_store_si256((__m256i*)(arr + j), minv);
                _mm256_store_si256((__m256i*)(arr + j + k), maxv);
            }
            else {
                _mm256_store_si256((__m256i*)(arr + j), maxv);
                _mm256_store_si256((__m256i*)(arr + j + k), minv);
            }
        }
    }
}

static inline void bitonic_merge(int32_t* arr, size_t len, int dir)
{
    for (size_t d = len / 2; d >= 8; d >>= 1)
    {
        compare_and_swap_avx(arr, d, len, dir);
    }

    size_t start_d = (len >= 8) ? 4 : len / 2;
    for (size_t d = start_d; d > 0; d >>= 1)
    {
        for (size_t i = 0; i < len; i++)
        {
            size_t ixj = i ^ d;
            if (ixj > i)
            {
                if ((arr[i] > arr[ixj]) == dir)
                {
                    swap(&arr[i], &arr[ixj]);
                }
            }
        }
    }
}

void bitonic_sort_avx(int32_t* arr, size_t len, int dir)
{
    if (len < 2) {
        return;
    }

    size_t padded_len = get_next_power_of_two(len);
    if (padded_len < 8) padded_len = 8;

    int32_t* padded_arr = (int32_t*)_mm_malloc(padded_len * sizeof(int32_t), 32);
    if (!padded_arr) return;

    int32_t dummy_val = dir ? INT32_MAX : INT32_MIN;
    memcpy(padded_arr, arr, len * sizeof(int32_t));
    for (size_t i = len; i < padded_len; ++i) padded_arr[i] = dummy_val;

    for (size_t size = 2; size <= padded_len; size <<= 1)
    {
        for (size_t i = 0; i < padded_len; i += size)
        {
            int sub_dir = ((i / size) % 2 == 0) ? 1 : 0;
            bitonic_merge(padded_arr + i, size, sub_dir);
        }
    }
    bitonic_merge(padded_arr, padded_len, dir);

    memcpy(arr, padded_arr, len * sizeof(int32_t));

    _mm_free(padded_arr);
}