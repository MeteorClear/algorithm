#include "CheckSIMD.h"

#ifdef _MSC_VER
#   include <intrin.h>
#else
#   include <cpuid.h>
#endif


/*
* Check SSE (Streaming SIMD Extensions) family capabilities.
* return: bitset(uint32)
* 0: SSE
* 1: SSE2
* 2: SSE3
* 3: SSSE3
* 4: SSE4.1
* 5: SSE4.2
*/
extern "C" uint32_t check_SSE()
{
    static int SSE_CACHED = 0;
    static uint32_t SSE_CAPS = 0;

    if (SSE_CACHED) return SSE_CAPS;

    int cpu_info[4] = { 0 }; // EAX, EBX, ECX, EDX

#if defined(_MSC_VER)
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]),
        "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    enum {
        SSE_CAP_SSE   = 1u << 0,      // SSE
        SSE_CAP_SSE2  = 1u << 1,      // SSE2
        SSE_CAP_SSE3  = 1u << 2,      // SSE3
        SSE_CAP_SSSE3 = 1u << 3,      // SSSE3
        SSE_CAP_SSE41 = 1u << 4,      // SSE4.1
        SSE_CAP_SSE42 = 1u << 5,      // SSE4.2
    };

    uint32_t bits = 0;

    // SSE  (EDX bit 25)
    if (cpu_info[3] & (1u << 25)) bits |= SSE_CAP_SSE;

    // SSE2 (EDX bit 26)
    if (cpu_info[3] & (1u << 26)) bits |= SSE_CAP_SSE2;

    // SSE3 (ECX bit 0)
    if (cpu_info[2] & (1u << 0)) bits |= SSE_CAP_SSE3;

    // SSSE3 (ECX bit 9)
    if (cpu_info[2] & (1u << 9)) bits |= SSE_CAP_SSSE3;

    // SSE4.1 (ECX bit 19)
    if (cpu_info[2] & (1u << 19)) bits |= SSE_CAP_SSE41;

    // SSE4.2 (ECX bit 20)
    if (cpu_info[2] & (1u << 20)) bits |= SSE_CAP_SSE42;

    SSE_CAPS = bits;
    SSE_CACHED = 1;
    return SSE_CAPS;
}

/*
* Check AMD XOP (SSE4a, SSE5/XOP) and FMA (FMA3/FMA4) capabilities.
* return: bitset(uint32)
* 0: SSE4a
* 1: XOP / SSE5
* 2: FMA3
* 3: FMA4
*/
extern "C" uint32_t check_AMD()
{
    static int AMD_CACHED = 0;
    static uint32_t AMD_CAPS = 0;

    if (AMD_CACHED) return AMD_CAPS;

    int cpu_info[4] = { 0 };   // EAX, EBX, ECX, EDX
    int ext_info[4] = { 0 };   // Extended (AMD leaf)

#if defined(_MSC_VER)
    __cpuid(cpu_info, 1);
    __cpuid(ext_info, 0x80000001);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]),
        "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
    __asm__ __volatile__(
        "cpuid"
        : "=a"(ext_info[0]), "=b"(ext_info[1]),
        "=c"(ext_info[2]), "=d"(ext_info[3])
        : "a"(0x80000001)
    );
#endif

    enum {
        AMD_CAP_SSE4A = 1u << 0,      // AMD SSE4a
        AMD_CAP_XOP   = 1u << 1,      // AMD XOP
        AMD_CAP_FMA3  = 1u << 2,      // Intel/AMD FMA3
        AMD_CAP_FMA4  = 1u << 3,      // AMD FMA4
    };

    uint32_t bits = 0;

    // AMD SSE4a (ECX bit 6)
    if (ext_info[2] & (1u << 6)) bits |= AMD_CAP_SSE4A;

    // AMD XOP (a.k.a. SSE5, deprecated) (ECX bit 11)
    if (ext_info[2] & (1u << 11)) bits |= AMD_CAP_XOP;

    // Intel/AMD FMA3 (ECX bit 12)
    if (cpu_info[2] & (1u << 12)) bits |= AMD_CAP_FMA3;

    // AMD FMA4 (ECX bit 15)
    if (ext_info[2] & (1u << 15)) bits |= AMD_CAP_FMA4;

    AMD_CAPS = bits;
    AMD_CACHED = 1;
    return AMD_CAPS;
}

/*
* Check AVX(Advanced Vector eXtensions) family capabilities.
* return: bitset(uint32)
* 0: AVX (base)
* 1: AVX2
* 2: AVX-VNNI
* 3: AVX-VNNI-INT8
* 4: AVX-VNNI-FP16
* 5: AVX-IFMA
*/
extern "C" uint32_t check_AVX()
{
    static int AVX_CACHED = 0;
    static uint32_t AVX_CAPS = 0;

    if (AVX_CACHED) return AVX_CAPS;

    int cpu_info[4];  // EAX, EBX, ECX, EDX

#if defined(_MSC_VER)
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    const int OSXSAVE_bit = (cpu_info[2] & (1 << 27)) ? 1 : 0;  // OSXSAVE
    const int AVX_bit = (cpu_info[2] & (1 << 28)) ? 1 : 0;  // AVX

    // OS support XSAVE/SRESTORE, CPU support AVX
    if (!(OSXSAVE_bit && AVX_bit)) {
        AVX_CACHED = 1;
        AVX_CAPS = 0;
        return AVX_CAPS;
    }

    // Check XGETBV for XMM/YMM (bit1, bit2)
    unsigned long long xcr0 = 0;
#if defined(_MSC_VER)
    xcr0 = _xgetbv(0);
#else
    unsigned int eax, edx;
    __asm__ __volatile__("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
    xcr0 = ((unsigned long long)edx << 32) | eax;
#endif

    if ((xcr0 & 0x6) != 0x6) {  // 0x6 = 0110b (Bits 1, 2)
        AVX_CACHED = 1;
        AVX_CAPS = 0;
        return AVX_CAPS;
    }

    enum {
        AVX_CAP_BASE = 1u << 0,      // AVX Base
        AVX_CAP_2    = 1u << 1,      // AVX 2
        AVX_CAP_VNNI = 1u << 2,      // AVX-VNNI
        AVX_CAP_INT8 = 1u << 3,      // AVX-VNNI-INT8
        AVX_CAP_FP16 = 1u << 4,      // AVX-VNNI-FP16
        AVX_CAP_IFMA = 1u << 5,      // AVX-IFMA
    };

    uint32_t bits = 0;

    // AVX base (ECX bit 28)
    if (cpu_info[2] & (1 << 28)) bits |= AVX_CAP_BASE;

    // Extended feature leaf 7
#if defined(_MSC_VER)
    __cpuidex(cpu_info, 7, 0);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(7), "c"(0)
    );
#endif

    // AVX2 (EBX bit 5)
    if (cpu_info[1] & (1u << 5)) bits |= AVX_CAP_2;

    // AVX-VNNI (ECX bit 11)
    if (cpu_info[2] & (1u << 11)) bits |= AVX_CAP_VNNI;

    // AVX-VNNI-INT8 (EDX bit 4)
    if (cpu_info[3] & (1u << 4)) bits |= AVX_CAP_INT8;

    // AVX-VNNI-FP16 (EDX bit 5)
    if (cpu_info[3] & (1u << 5)) bits |= AVX_CAP_FP16;

    // AVX-IFMA (EBX bit 21)
    if (cpu_info[1] & (1u << 21)) bits |= AVX_CAP_IFMA;

    AVX_CAPS = bits;
    AVX_CACHED = 1;
    return AVX_CAPS;
}

/*
* Check AVX512 family capabilities.
* return: bitset(uint32)
* 0: Foundation
* 1: Double/Quadword
* 2: Integer FMA
* 3: Prefetch
* 4: Exponential / Reciprocal
* 5: Conflict Detection
* 6: Byte/Word
* 7: Vector Length (128/256)
*/
extern "C" uint32_t check_AVX512()
{
    static int AVX512_CACHED = 0;
    static uint32_t AVX512_CAPS = 0;

    if (AVX512_CACHED) return AVX512_CAPS;

    int cpu_info[4];  // EAX, EBX, ECX, EDX

#if defined(_MSC_VER)
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    int OSXSAVE_bit = (cpu_info[2] & (1 << 27)) ? 1 : 0;  // OSXSAVE (bit 27 of ECX)
    int AVX_bit = (cpu_info[2] & (1 << 28)) ? 1 : 0;  // AVX (bit 28 of ECX)

    // OS support XSAVE/SRESTORE, CPU support AVX command
    if (!(OSXSAVE_bit && AVX_bit)) {
        AVX512_CAPS = 0;
        AVX512_CACHED = 1;
        return AVX512_CAPS;
    }

    // Check the XGETBV(XCR0) (XCR0 bit 1: XMM, bit 2: YMM, bit5,6: ZMM)
    unsigned long long xcr0 = 0;
#if defined(_MSC_VER)
    xcr0 = _xgetbv(0);
#else
    unsigned int eax_x, edx_x;
    __asm__ __volatile__("xgetbv" : "=a"(eax_x), "=d"(edx_x) : "c"(0));
    xcr0 = ((unsigned long long)edx_x << 32) | eax_x;
#endif

    // SSE(XMM, bit1), AVX(YMM, bit2), AVX-512 ZMM (bit 5,6)
    if ((xcr0 & 0x66u) != 0x66u) { // 0x66 = 0110 0110b (Bits 1, 2, 5, 6)
        AVX512_CAPS = 0;
        AVX512_CACHED = 1;
        return AVX512_CAPS;
    }

    // Check the AVX bit on leaf 7
#if defined(_MSC_VER)
    __cpuidex(cpu_info, 7, 0);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(7), "c"(0)
    );
#endif

    enum {
        AVX512_CAP_F    = 1u << 0,      // Foundation
        AVX512_CAP_DQ   = 1u << 1,      // Double/Quadword
        AVX512_CAP_IFMA = 1u << 2,      // Integer FMA
        AVX512_CAP_PF   = 1u << 3,      // Prefetch
        AVX512_CAP_ER   = 1u << 4,      // Exponential / Reciprocal
        AVX512_CAP_CD   = 1u << 5,      // Conflict Detection
        AVX512_CAP_BW   = 1u << 6,      // Byte/Word
        AVX512_CAP_VL   = 1u << 7       // Vector Length (128/256)
    };

    uint32_t bits = 0;
    if (cpu_info[1] & (1u << 16)) bits |= AVX512_CAP_F;     // AVX-512 Foundation
    if (cpu_info[1] & (1u << 17)) bits |= AVX512_CAP_DQ;    // DQ
    if (cpu_info[1] & (1u << 21)) bits |= AVX512_CAP_IFMA;  // IFMA
    if (cpu_info[1] & (1u << 26)) bits |= AVX512_CAP_PF;    // Prefetch
    if (cpu_info[1] & (1u << 27)) bits |= AVX512_CAP_ER;    // ER
    if (cpu_info[1] & (1u << 28)) bits |= AVX512_CAP_CD;    // CD
    if (cpu_info[1] & (1u << 30)) bits |= AVX512_CAP_BW;    // BW
    if (cpu_info[1] & (1u << 31)) bits |= AVX512_CAP_VL;    // VL

    AVX512_CAPS = bits;
    AVX512_CACHED = 1;

    return AVX512_CAPS;
}

/*
* Check all SIMD instruction set capabilities (Intel + AMD).
* Return: bitset(uint64)
*
* Bit layout:
*  0–5   : SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2
*  8–11  : AMD extensions (SSE4a, XOP, FMA3, FMA4)
*  16–21 : AVX, AVX2, AVX-VNNI, AVX-VNNI-INT8, AVX-VNNI-FP16, AVX-IFMA
*  32–39 : AVX-512 family (F, DQ, IFMA, PF, ER, CD, BW, VL)
*/
extern "C" uint64_t check_SIMD()
{
    uint64_t caps = 0;

    uint32_t sse    = check_SSE();
    uint32_t amd    = check_AMD();
    uint32_t avx    = check_AVX();
    uint32_t avx512 = check_AVX512();

    enum {
        SIMD_OFFSET_SSE    = 0,
        SIMD_OFFSET_AMD    = 8,
        SIMD_OFFSET_AVX    = 16,
        SIMD_OFFSET_AVX512 = 32
    };

    // Map into unified bit space
    caps |= (uint64_t)(sse & 0x3F)    << SIMD_OFFSET_SSE;     // SSE0–5
    caps |= (uint64_t)(amd & 0x0F)    << SIMD_OFFSET_AMD;     // AMD0–3
    caps |= (uint64_t)(avx & 0x3F)    << SIMD_OFFSET_AVX;     // AVX0–5
    caps |= (uint64_t)(avx512 & 0xFF) << SIMD_OFFSET_AVX512;  // AVX5120–7

    return caps;
}


/*
// Example
int main()
{
    enum {
        SIMD_OFFSET_SSE = 0,
        SIMD_OFFSET_AMD = 8,
        SIMD_OFFSET_AVX = 16,
        SIMD_OFFSET_AVX512 = 32
    };

    uint64_t simd = check_SIMD();

    printf("=== SIMD Capability Detection ===\n");

    // SSE
    printf("[SSE]\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 0))) printf("  - SSE supported\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 1))) printf("  - SSE2 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 2))) printf("  - SSE3 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 3))) printf("  - SSSE3 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 4))) printf("  - SSE4.1 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_SSE + 5))) printf("  - SSE4.2 supported\n");

    // AMD
    printf("[AMD Extensions]\n");
    if (simd & (1ull << (SIMD_OFFSET_AMD + 0))) printf("  - SSE4a supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AMD + 1))) printf("  - XOP / SSE5 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AMD + 2))) printf("  - FMA3 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AMD + 3))) printf("  - FMA4 supported\n");

    // AVX
    printf("[AVX]\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 0))) printf("  - AVX supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 1))) printf("  - AVX2 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 2))) printf("  - AVX-VNNI supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 3))) printf("  - AVX-VNNI-INT8 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 4))) printf("  - AVX-VNNI-FP16 supported\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX + 5))) printf("  - AVX-IFMA supported\n");

    // AVX512
    printf("[AVX-512]\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 0))) printf("  - AVX-512 Foundation\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 1))) printf("  - AVX-512 DQ (Double/Quadword)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 2))) printf("  - AVX-512 IFMA (Integer FMA)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 3))) printf("  - AVX-512 PF (Prefetch)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 4))) printf("  - AVX-512 ER (Exponential/Reciprocal)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 5))) printf("  - AVX-512 CD (Conflict Detection)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 6))) printf("  - AVX-512 BW (Byte/Word)\n");
    if (simd & (1ull << (SIMD_OFFSET_AVX512 + 7))) printf("  - AVX-512 VL (Vector Length 128/256)\n");

    printf("\nRaw Bitset: 0x%016llX\n", (unsigned long long)simd);

    return 0;
}
*/