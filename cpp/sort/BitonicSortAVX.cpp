/*
* Bitonic Sort(Bitonic mergesort) with AVX.
* It is a parallel algorithm for sorting used as a construction method for building a sorting network.
* In this implementation, parallel processing is implemented using AVX.
* 
*   Worst-case: O( (log n)^2 ) parallel time
*    Best-case: O( (log n)^2 ) parallel time
* Average-case: O( (log n)^2 ) parallel time
* 
*/

#include <immintrin.h>  // AVX
#include <stdint.h>     // int32_t
#include <stddef.h>     // size_t
#include <string.h>     // memcpy
#include <limits.h>     // INT32_MAX, INT32_MIN

/*
* Swap two int32 values.
*/
static inline void swap(int32_t* a, int32_t* b) { int32_t t = *a; *a = *b; *b = t; }

/*
* Get the next 2^i, greater than or equal to n.
*/
static inline size_t get_next_power_of_two(size_t n)
{
    if (n <= 1) return 1;               // Exception, 2^0
    if ((n & (n - 1)) == 0) return n;   // Exception, already 2^i

    // Fill all bits below the highest set bit
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFFUL
    n |= n >> 32;
#endif
    n++;    // n+1 = next 2^i

    return n;
}

/*
* Compare and Swap on two blocks seperated by distance k.
* dir = 1 (ascending), dir = 0 (descending).
*/
static inline void compare_and_swap_avx(int32_t* arr, size_t k, size_t len, int dir)
{
    // Iterate through pairs of bitonic subsequences spaced 2*k apart
    for (size_t i = 0; i < len; i += 2 * k)
    {
        // Within each half, compare 8 elements at a time using AVX, ( 256(AVX width) / 32(int width) = 8 )
        for (size_t j = i; j + 8 <= i + k; j += 8)
        {
            // Load vectors
            __m256i va = _mm256_load_si256((__m256i*)(arr + j));
            __m256i vb = _mm256_load_si256((__m256i*)(arr + j + k));
            // Compute element-wise minmax
            __m256i minv = _mm256_min_epi32(va, vb);
            __m256i maxv = _mm256_max_epi32(va, vb);
            // Store results according to sorting direction
            if (dir)
            {
                // Ascending: smaller values go to the first half
                _mm256_store_si256((__m256i*)(arr + j), minv);
                _mm256_store_si256((__m256i*)(arr + j + k), maxv);
            }
            else
            {
                // Descending: larger values go to the first half
                _mm256_store_si256((__m256i*)(arr + j), maxv);
                _mm256_store_si256((__m256i*)(arr + j + k), minv);
            }
        }
    }
}

/*
* Bitonic merge operation on array.
* dir = 1 for ascending merge, 0 for descending merge.
*/
static inline void bitonic_merge(int32_t* arr, size_t len, int dir)
{
    // AVX-based merge for large strides (>=8 elements)
    for (size_t d = len / 2; d >= 8; d >>= 1)
    {
        compare_and_swap_avx(arr, d, len, dir);
    }

    // Scalar merge for small strides (<8 elements)
    size_t start_d = (len >= 8) ? 4 : len / 2;
    for (size_t d = start_d; d > 0; d >>= 1)
    {
        // Compare and swap each element pair (i, i^d)
        for (size_t i = 0; i < len; i++)
        {
            size_t ixj = i ^ d;     // XOR to find comparison partner
            if (ixj > i)            // Ensure each pair is processed once
            {
                // Swap if order is wrong according to direction
                if ((arr[i] > arr[ixj]) == dir) swap(&arr[i], &arr[ixj]);
            }
        }
    }
}

/*
* Bitonic Sort using AVX.
* arr: input array.
* len: num of elements.
* dir: 1 = ascending order, 0 = descending order.
* No return, but modify the input array directly.
*/
void bitonic_sort_avx(int32_t* arr, size_t len, int dir)
{
    if (len < 2) return;                                                    // Exception, single element
    
    // Bitonic sort work only 2^i length
    // Ensure total length is 2^i for bitonic sorting
    size_t padded_len = get_next_power_of_two(len);
    if (padded_len < 8) padded_len = 8;                                     // Minimum vectorized size

    // Allocate 32-byte aligned buffer for AVX operations
    int32_t* padded_arr = (int32_t*)_mm_malloc(padded_len * sizeof(int32_t), 32);
    if (!padded_arr) return;                                                // Exception, allocation failed

    // Fill padding with dummy values
    int32_t dummy_val = dir ? INT32_MAX : INT32_MIN;                        // Dummy value depending on direction
    memcpy(padded_arr, arr, len * sizeof(int32_t));                         // Copy original data
    for (size_t i = len; i < padded_len; ++i) padded_arr[i] = dummy_val;    // Pad dummy value

    // Build bitonic sequences of increasing size
    for (size_t size = 2; size <= padded_len; size <<= 1)
    {
        // Process each sub-block of the current size
        for (size_t i = 0; i < padded_len; i += size)
        {
            // Set direction, first half ascending, second half descending
            int sub_dir = ((i / size) % 2 == 0) ? 1 : 0;
            bitonic_merge(padded_arr + i, size, sub_dir);
        }
    }

    // Final merge across the full array
    bitonic_merge(padded_arr, padded_len, dir);

    // Copy sorted elements without padding to the original array
    memcpy(arr, padded_arr, len * sizeof(int32_t));

    // Free aligned buffer
    _mm_free(padded_arr);
}

/*
* Improved Bitonic Sort Example
* Goal: Sort the initial array in Ascending (ASC) order.
*
* init array
* [8, 1, 6, 3, 7, 2, 5, 4]
*
* Step 1: Bitonic Sequence Construction
* sort 2^k blocks to form 2^{k+1} bitonic sequences
* blocks are merged using alternating sort directions to maintain bitonic property
*
* Step 1-1: Build 2^1 blocks (Size 2, all ASC)
* [8, 1] (ASC) -> [1, 8]
* [6, 3] (ASC) -> [3, 6]
* [7, 2] (ASC) -> [2, 7]
* [5, 4] (ASC) -> [4, 5]
* result
* [1, 8, 3, 6, 2, 7, 4, 5]
*
* Step 1-2: Build 2^2 bitonic sequence (Size 4, ASC & DESC merge)
* [1, 8, 3, 6] (ASC)  -> [1, 3, 6, 8]
* [2, 7, 4, 5] (DESC) -> [7, 5, 4, 2]
* result
* [1, 3, 6, 8, 7, 5, 4, 2]
*
* Step 2: Bitonic Merge (Final Sorting Phase)
* bitonic sequence is merged into a fully sorted sequence
* this involves iterative compare-swap operations with decreasing distances
*
* Step 2-1: Compare-swap distance 4
* pair index (0,4), (1,5), (2,6), (3,7)
* (1, 7) -> OK
* (3, 5) -> OK
* (6, 4) -> SWAP -> [4, 6]
* (8, 2) -> SWAP -> [2, 8]
* result
* [1, 3, 4, 2, 7, 5, 6, 8]
*
* Step 2-2: Compare-swap distance 2
* pair index (0,2), (1,3), (4,6), (5,7)
* (1, 4) -> OK
* (3, 2) -> SWAP -> [2, 3]
* (7, 6) -> SWAP -> [6, 7]
* (5, 8) -> OK
* result
* [1, 2, 4, 3, 6, 5, 7, 8]
*
* Step 2-3: Compare-swap distance 1
* pair index (0,1), (2,3), (4,5), (6,7)
* (1, 2) -> OK
* (4, 3) -> SWAP -> [3, 4]
* (6, 5) -> SWAP -> [5, 6]
* (7, 8) -> OK
* result
* [1, 2, 3, 4, 5, 6, 7, 8]
*
* Step 3: Final Bitonic Merge (size = 8)
*
* Result sorted array
* [1, 2, 3, 4, 5, 6, 7, 8]
*/
