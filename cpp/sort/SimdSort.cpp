#include <immintrin.h>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cstring>
#include <vector>
#include <intrin.h>

namespace simd_sort {
    static constexpr int  SMALL_SORT_THRESHOLD = 128;
    static constexpr int  BLOCK = 256;

    static inline void insertion_sort_int32(int32_t* a, size_t n) {
        for (size_t i = 1; i < n; ++i) {
            int32_t v = a[i];
            size_t j = i;
            while (j > 0 && a[j - 1] > v) {
                a[j] = a[j - 1];
                --j;
            }
            a[j] = v;
        }
    }

    static inline int32_t median3(int32_t a, int32_t b, int32_t c) {
        if (a < b) {
            if (b < c) return b;
            else if (a < c) return c;
            else return a;
        }
        else {
            if (a < c) return a;
            else if (b < c) return c;
            else return b;
        }
    }

    static inline int build_compact_indices(uint8_t mask, int idx[8]) {
        int k = 0;
        for (int lane = 0; lane < 8; ++lane) if (mask & (1u << lane)) idx[k++] = lane;
        for (int p = k; p < 8; ++p) idx[p] = p;
        return k;
    }

    struct MaskLut { __m256i idx; uint8_t k; };
    static inline const MaskLut& mask_lut(uint8_t m) {
        static MaskLut T[256];
        static bool inited = false;
        if (!inited) {
            for (int mm = 0; mm < 256; ++mm) {
                int idx[8], p = 0;
                for (int lane = 0; lane < 8; ++lane) if (mm & (1 << lane)) idx[p++] = lane;
                for (int lane = p; lane < 8; ++lane) idx[lane] = lane;
                T[mm].idx = _mm256_setr_epi32(idx[0], idx[1], idx[2], idx[3], idx[4], idx[5], idx[6], idx[7]);
                T[mm].k = (uint8_t)__popcnt(mm);
            }
            inited = true;
        }
        return T[m];
    }

    static inline void store_left_k(int32_t* a, ptrdiff_t& L, const int32_t* src, int k) {
        if (k >= 8) {
            _mm256_storeu_si256((__m256i*)(a + L), _mm256_load_si256((const __m256i*)src));
            L += 8; return;
        }
        if (k >= 4) {
            _mm_storeu_si128((__m128i*)(a + L), _mm_load_si128((const __m128i*)src));
            L += 4; src += 4; k -= 4;
        }
        while (k--) a[L++] = *src++;
    }


    static inline void flush_right_vec(int32_t* a, ptrdiff_t& R, const int32_t* src, int n) {
        while (n >= 8) {
            R -= 7;
            _mm256_storeu_si256((__m256i*)(a + R), _mm256_loadu_si256((const __m256i*)src));
            R -= 1; src += 8; n -= 8;
        }
        if (n >= 4) {
            R -= 3;
            _mm_storeu_si128((__m128i*)(a + R), _mm_loadu_si128((const __m128i*)src));
            R -= 1; src += 4; n -= 4;
        }
        while (n--) a[R--] = *src++;
    }


    static inline void fill_eq_block(int32_t* dst, size_t n, int32_t pivot) {
        __m256i pv = _mm256_set1_epi32(pivot);
        while (n >= 8) { _mm256_storeu_si256((__m256i*)dst, pv); dst += 8; n -= 8; }
        if (n >= 4) { _mm_storeu_si128((__m128i*)dst, _mm256_castsi256_si128(pv)); dst += 4; n -= 4; }
        while (n--) *dst++ = pivot;
    }

    static inline void append_k(int32_t* buf, int& idx, const int32_t* src, int k) {
        if (k >= 8) {
            _mm256_store_si256((__m256i*)(buf + idx),
                _mm256_load_si256((const __m256i*)src));
            idx += 8; return;
        }
        if (k >= 4) {
            _mm_storeu_si128((__m128i*)(buf + idx),
                _mm_loadu_si128((const __m128i*)src));
            idx += 4; src += 4; k -= 4;
        }
        while (k--) buf[idx++] = *src++;
    }

    static inline void simd_partition_int32_avx2(
        int32_t* a, ptrdiff_t left, ptrdiff_t right, int32_t pivot,
        ptrdiff_t& lt_end, ptrdiff_t& gt_begin)
    {
        const __m256i pv = _mm256_set1_epi32(pivot);
        ptrdiff_t L = left;
        ptrdiff_t R = right;
        ptrdiff_t i = left;

        alignas(64) int32_t rbuf[BLOCK];
        int rc = 0;
        size_t eq_count = 0;

        for (; i + 7 <= right; i += 8) {
            __m256i v = _mm256_loadu_si256((const __m256i*)(a + i));
            __m256i mlt = _mm256_cmpgt_epi32(pv, v);
            __m256i mgt = _mm256_cmpgt_epi32(v, pv);
            uint8_t ml = (uint8_t)_mm256_movemask_ps(_mm256_castsi256_ps(mlt));
            uint8_t mg = (uint8_t)_mm256_movemask_ps(_mm256_castsi256_ps(mgt));

            if (ml == 0xFF) {
                _mm256_storeu_si256((__m256i*)(a + L), v);
                L += 8;
                continue;
            }
            if (mg == 0xFF) {
                if (rc + 8 > BLOCK) { flush_right_vec(a, R, rbuf, rc); rc = 0; }
                _mm256_storeu_si256((__m256i*)(rbuf + rc), v);
                rc += 8;
                continue;
            }
            if ((ml | mg) == 0x00) {
                eq_count += 8;
                continue;
            }
            if (mg == 0x00) {
                const MaskLut& LM = mask_lut(ml);
                __m256i pack = _mm256_permutevar8x32_epi32(v, LM.idx);
                alignas(32) int32_t tmp[8];
                _mm256_store_si256((__m256i*)tmp, pack);
                store_left_k(a, L, tmp, LM.k);
                eq_count += (size_t)(8 - LM.k);
                continue;
            }
            if (ml == 0x00) {
                const MaskLut& GM = mask_lut(mg);
                __m256i pack = _mm256_permutevar8x32_epi32(v, GM.idx);
                alignas(32) int32_t tmp[8];
                _mm256_store_si256((__m256i*)tmp, pack);

                int off = 0;
                while (off < GM.k) {
                    if (rc == BLOCK) { flush_right_vec(a, R, rbuf, rc); rc = 0; }
                    int can = std::min(GM.k - off, BLOCK - rc);
                    append_k(rbuf, rc, tmp + off, can);
                    off += can;
                }
                eq_count += (size_t)(8 - GM.k);
                continue;
            }

            int k_lt = 0, k_gt = 0;

            if (ml) {
                const MaskLut& LM = mask_lut(ml);
                __m256i pack = _mm256_permutevar8x32_epi32(v, LM.idx);
                alignas(32) int32_t tmp[8];
                _mm256_store_si256((__m256i*)tmp, pack);
                store_left_k(a, L, tmp, LM.k);
                k_lt = LM.k;
            }

            if (mg) {
                const MaskLut& GM = mask_lut(mg);
                __m256i pack = _mm256_permutevar8x32_epi32(v, GM.idx);
                alignas(32) int32_t tmp[8];
                _mm256_store_si256((__m256i*)tmp, pack);

                int off = 0;
                while (off < GM.k) {
                    if (rc == BLOCK) { flush_right_vec(a, R, rbuf, rc); rc = 0; }
                    int can = std::min(GM.k - off, BLOCK - rc);
                    append_k(rbuf, rc, tmp + off, can);
                    off += can;
                }
                k_gt = GM.k;
            }

            eq_count += (size_t)(8 - (k_lt + k_gt));
        }

        for (; i <= right; ++i) {
            int32_t x = a[i];
            if (x < pivot) { a[L++] = x; }
            else if (x == pivot) { ++eq_count; }
            else {
                if (rc == BLOCK) { flush_right_vec(a, R, rbuf, rc); rc = 0; }
                rbuf[rc++] = x;
            }
        }

        if (rc) { flush_right_vec(a, R, rbuf, rc); rc = 0; }

        ptrdiff_t num_lt = L - left;
        if (eq_count) { fill_eq_block(a + L, eq_count, pivot); L += (ptrdiff_t)eq_count; }

        lt_end = (num_lt > 0) ? (left + num_lt - 1) : (left - 1);
        gt_begin = R + 1;
    }

    static void qsort_int32_core(int32_t* a, ptrdiff_t left, ptrdiff_t right) {
        while (left < right) {
            ptrdiff_t n = right - left + 1;
            if (n <= SMALL_SORT_THRESHOLD) {
                insertion_sort_int32(a + left, (size_t)n);
                return;
            }

            const int32_t pivot = median3(a[left], a[left + n / 2], a[right]);

            ptrdiff_t lt_end, gt_begin;
            simd_partition_int32_avx2(a, left, right, pivot, lt_end, gt_begin);

            const ptrdiff_t left_n = (lt_end >= left) ? (lt_end - left + 1) : 0;
            const ptrdiff_t right_n = (gt_begin <= right) ? (right - gt_begin + 1) : 0;

            if (left_n < right_n) {
                if (left_n > 1) qsort_int32_core(a, left, lt_end);
                left = gt_begin;
            }
            else {
                if (right_n > 1) qsort_int32_core(a, gt_begin, right);
                right = lt_end;
            }
        }
    }

    // ---------- Public API ----------
    inline void simd_qsort_int32(int32_t* a, size_t n) {
        if (!a || n <= 1) return;
        qsort_int32_core(a, 0, (ptrdiff_t)n - 1);
    }
}