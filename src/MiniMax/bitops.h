#ifndef BITOPS_H
#define BITOPS_H

#include <stdint.h>

/* Portable count-trailing-zeros for 64-bit integers */
#if defined(__GNUC__) || defined(__clang__)
#define HAS_CTZ64
#define CTZ64(x) __builtin_ctzll(x)
#elif defined(_MSC_VER) && defined(_WIN64)
#include <intrin.h>
#define HAS_CTZ64
static inline int CTZ64(uint64_t x)
{
    unsigned long index;
    _BitScanForward64(&index, x);
    return (int)index;
}
#endif

#endif
