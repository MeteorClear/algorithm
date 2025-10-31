#include "CheckSIMD.h"

#ifdef _MSC_VER
#   include <intrin.h>
#else
#   include <cpuid.h>
#endif

static int SSE_LEVEL = -1;  // -1 = unknown, 0 = unsupport, 1 = SSE, 2 = SSE2, 3 = SSE3, 4 = SSSE3, 5 = SSE4.1, 6 = SSE4.2
static int XOP_LEVEL = -1;  // -1 = unknown, 0 = unsupport, 1 = SSE4a, 2 = XOP(SSE5)
static int FMA_LEVEL = -1;  // -1 = unknown, 0 = unsupport, 1 = FMA3, 2 = FMA4
static int AVX_LEVEL = -1;  // -1 = unknown, 0 = unsupport, 1 = AVX, 2 = AVX2

static int AVX512_CACHED = 0;
static unsigned int AVX512_CAPS = 0;

/*
* Check SSE(Streaming SIMD Extensions) compatibility by referring to the register bits in cpu.
* return:
* -1 = unknown, 0 = unsupport, 1 = SSE, 2 = SSE2, 3 = SSE3, 4 = SSSE3, 5 = SSE4.1, 6 = SSE4.2
*/
extern "C" int check_SSE_level()
{
    if (SSE_LEVEL != -1) return SSE_LEVEL;

    int cpu_info[4] = { 0 };  // EAX, EBX, ECX, EDX

#ifdef _MSC_VER
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    const int SSE4_2bit = (cpu_info[2] & (1 << 20)) ? 1 : 0;  // SSE4.2 (bit 20 of ECX)
    if (SSE4_2bit) 
    {
        SSE_LEVEL = 6;
        return SSE_LEVEL;
    }

    const int SSE4_1bit = (cpu_info[2] & (1 << 19)) ? 1 : 0;  // SSE4.1 (bit 19 of ECX)
    if (SSE4_1bit) 
    {
        SSE_LEVEL = 5;
        return SSE_LEVEL;
    }

    const int SSSE3_bit = (cpu_info[2] & (1 << 9)) ? 1 : 0;  // SSSE3 (bit 9 of ECX)
    if (SSSE3_bit)
    {
        SSE_LEVEL = 4;
        return SSE_LEVEL;
    }

    const int SSE3_bit = (cpu_info[2] & (1 << 0)) ? 1 : 0;  // SSE3 (bit 0 of ECX)
    if (SSE3_bit)
    {
        SSE_LEVEL = 3;
        return SSE_LEVEL;
    }

    const int SSE2_bit = (cpu_info[3] & (1 << 26)) ? 1 : 0;  // SSE2 (bit 26 of EDX)
    if (SSE2_bit)
    {
        SSE_LEVEL = 2;
        return SSE_LEVEL;
    }

    const int SSE_bit = (cpu_info[3] & (1 << 25)) ? 1 : 0;  // SSE (bit 25 of EDX)
    if (SSE_bit)
    {
        SSE_LEVEL = 1;
        return SSE_LEVEL;
    }

    SSE_LEVEL = 0;

    return SSE_LEVEL;
}

/*
* Check XOP(eXtended OPerations) compatibility by referring to the register bits in cpu.
* return:
* -1 = unknown, 0 = unsupport, 1 = SSE4a, 2 = XOP(SSE5)
*/
extern "C" int check_XOP_level()
{
    if (XOP_LEVEL != -1) return XOP_LEVEL;

    int ext_info[4] = { 0 };  // AMD (leaf 0x80000001)

#ifdef _MSC_VER
    __cpuid(ext_info, 0x80000001);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(ext_info[0]), "=b"(ext_info[1]), "=c"(ext_info[2]), "=d"(ext_info[3])
        : "a"(0x80000001)
    );
#endif

    const int SSE5_bit = (ext_info[2] & (1 << 11)) ? 1 : 0;  // AMD SSE5(XOP) (bit 11 of ECX), deprecated
    if (SSE5_bit)
    {
        XOP_LEVEL = 2;
        return XOP_LEVEL;
    }

    const int SSE4A_bit = (ext_info[2] & (1 << 6)) ? 1 : 0;  // AMD SSE4a (bit 5 of ECX)
    if (SSE4A_bit)
    {
        XOP_LEVEL = 1;
        return XOP_LEVEL;
    }

    XOP_LEVEL = 0;
    return XOP_LEVEL;
}

/*
* Check FMA(Fused Multiply-Add) compatibility by referring to the register bits in cpu.
* return:
* -1 = unknown, 0 = unsupport, 1 = FMA3, 2 = FMA4
*/
extern "C" int check_FMA_level()
{
    if (FMA_LEVEL != -1) return FMA_LEVEL;

    int cpu_info[4] = { 0 };  // EAX, EBX, ECX, EDX

#ifdef _MSC_VER
    __cpuid(cpu_info, 1);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(cpu_info[0]), "=b"(cpu_info[1]), "=c"(cpu_info[2]), "=d"(cpu_info[3])
        : "a"(1)
    );
#endif

    int ext_info[4] = { 0 };  // AMD (leaf 0x80000001)

#ifdef _MSC_VER
    __cpuid(ext_info, 0x80000001);
#else
    __asm__ __volatile__(
        "cpuid"
        : "=a"(ext_info[0]), "=b"(ext_info[1]), "=c"(ext_info[2]), "=d"(ext_info[3])
        : "a"(0x80000001)
    );
#endif

    int FMA4_bit = (ext_info[2] & (1 << 15)) ? 1 : 0; // ECX bit 15
    if (FMA4_bit)
    {
        FMA_LEVEL = 2;
        return FMA_LEVEL;
    }

    int FMA3_bit = (cpu_info[2] & (1 << 12)) ? 1 : 0; // ECX bit 12
    if (FMA3_bit)
    {
        FMA_LEVEL = 1;
        return FMA_LEVEL;
    }

    return FMA_LEVEL;
}

/*
* Check AVX(Advanced Vector eXtensions) compatibility by referring to the register bits in cpu.
* return:
* -1 = unknown, 0 = unsupport, 1 = AVX, 2 = AVX2
*/
extern "C" int check_AVX_level()
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

    int OSXSAVE_bit = (cpu_info[2] & (1 << 27)) ? 1 : 0;  // OSXSAVE (bit 27 of ECX)
    int AVX_bit = (cpu_info[2] & (1 << 28)) ? 1 : 0;  // AVX (bit 28 of ECX)

    // OS support XSAVE/SRESTORE, CPU support AVX command
    if (!(OSXSAVE_bit && AVX_bit)) {
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
    int AVX2_bit = (cpu_info[1] & (1 << 5)) ? 2 : 1;  // AVX2 (bit 5 of EBX)
    AVX_LEVEL = AVX2_bit;
    return AVX_LEVEL;
}

/*
* Check AVX512 compatibility by referring to the register bits in cpu.
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
extern "C" unsigned int check_AVX512()
{
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

    int caps = 0;
    if (cpu_info[1] & (1u << 16)) caps |= AVX512_CAP_F;     // AVX-512 Foundation
    if (cpu_info[1] & (1u << 17)) caps |= AVX512_CAP_DQ;    // DQ
    if (cpu_info[1] & (1u << 21)) caps |= AVX512_CAP_IFMA;  // IFMA
    if (cpu_info[1] & (1u << 26)) caps |= AVX512_CAP_PF;    // Prefetch
    if (cpu_info[1] & (1u << 27)) caps |= AVX512_CAP_ER;    // ER
    if (cpu_info[1] & (1u << 28)) caps |= AVX512_CAP_CD;    // CD
    if (cpu_info[1] & (1u << 30)) caps |= AVX512_CAP_BW;    // BW
    if (cpu_info[1] & (1u << 31)) caps |= AVX512_CAP_VL;    // VL

    AVX512_CAPS = caps;
    AVX512_CACHED = 1;

    return AVX512_CAPS;
}

extern "C" void init_SIMD_LEVEL()
{
    SSE_LEVEL = -1;
    XOP_LEVEL = -1;
    FMA_LEVEL = -1;
    AVX_LEVEL = -1;
    AVX512_CACHED = 0;
    AVX512_CAPS = 0;
}

/* // Example
int main()
{
    printf("=== CPU SIMD Feature Detection ===\n\n");

    // 1. SSE
    int sse = check_SSE_level();
    printf("SSE level : ");
    switch (sse) {
    case 0: printf("No SSE support\n"); break;
    case 1: printf("SSE\n"); break;
    case 2: printf("SSE2\n"); break;
    case 3: printf("SSE3\n"); break;
    case 4: printf("SSSE3\n"); break;
    case 5: printf("SSE4.1\n"); break;
    case 6: printf("SSE4.2\n"); break;
    default: printf("Unknown (%d)\n", sse); break;
    }

    // 2. XOP (AMD)
    int xop = check_XOP_level();
    printf("XOP level : ");
    switch (xop) {
    case 0: printf("Not supported\n"); break;
    case 1: printf("SSE4a\n"); break;
    case 2: printf("SSE5/XOP\n"); break;
    default: printf("Unknown (%d)\n", xop); break;
    }

    // 3. FMA (FMA3/FMA4)
    int fma = check_FMA_level();
    printf("FMA level : ");
    switch (fma) {
    case 0: printf("Not supported\n"); break;
    case 1: printf("FMA3 supported\n"); break;
    case 2: printf("FMA4 supported (AMD)\n"); break;
    default: printf("Unknown (%d)\n", fma); break;
    }

    // 4. AVX / AVX2
    int avx = check_AVX_level();
    printf("AVX level : ");
    switch (avx) {
    case 0: printf("No AVX support\n"); break;
    case 1: printf("AVX supported\n"); break;
    case 2: printf("AVX2 supported\n"); break;
    default: printf("Unknown (%d)\n", avx); break;
    }

    // 5. AVX-512 Subset
    unsigned int avx512 = check_AVX512();
    printf("AVX-512 capabilities:\n");
    if (avx512 == 0) {
        printf("  AVX-512 not supported.\n");
    }
    else {
        enum {
            AVX512_CAP_F = 1u << 0,      // Foundation
            AVX512_CAP_DQ = 1u << 1,      // Double/Quadword
            AVX512_CAP_IFMA = 1u << 2,      // Integer FMA
            AVX512_CAP_PF = 1u << 3,      // Prefetch
            AVX512_CAP_ER = 1u << 4,      // Exponential / Reciprocal
            AVX512_CAP_CD = 1u << 5,      // Conflict Detection
            AVX512_CAP_BW = 1u << 6,      // Byte/Word
            AVX512_CAP_VL = 1u << 7       // Vector Length (128/256)
        };

        printf("  AVX-512F   : %s\n", (avx512 & AVX512_CAP_F) ? "Yes" : "No");
        printf("  AVX-512DQ  : %s\n", (avx512 & AVX512_CAP_DQ) ? "Yes" : "No");
        printf("  AVX-512IFMA: %s\n", (avx512 & AVX512_CAP_IFMA) ? "Yes" : "No");
        printf("  AVX-512PF  : %s\n", (avx512 & AVX512_CAP_PF) ? "Yes" : "No");
        printf("  AVX-512ER  : %s\n", (avx512 & AVX512_CAP_ER) ? "Yes" : "No");
        printf("  AVX-512CD  : %s\n", (avx512 & AVX512_CAP_CD) ? "Yes" : "No");
        printf("  AVX-512BW  : %s\n", (avx512 & AVX512_CAP_BW) ? "Yes" : "No");
        printf("  AVX-512VL  : %s\n", (avx512 & AVX512_CAP_VL) ? "Yes" : "No");
    }

    printf("\n===================================\n");

    return 0;
}
*/