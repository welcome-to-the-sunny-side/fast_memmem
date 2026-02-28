#include <bit>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include <immintrin.h>
#include "fast_memmem.hpp"

namespace fm_SWAR
{
    static inline uint64_t byte_eq_mask(uint64_t z) 
    {
        return (z - 0x0101010101010101ULL) & ~z & 0x8080808080808080ULL;
    }
    static inline uint64_t match_mask64(uint64_t a, uint64_t b, uint64_t x, uint64_t y) 
    {
        uint64_t ma = byte_eq_mask(a ^ x);   // 0x80 in byte i iff a[i] == x[i]
        uint64_t mb = byte_eq_mask(b ^ y);   // 0x80 in byte i iff b[i] == y[i]
        return ma & mb;                      // 0x80 in byte i iff both comparisons match
    }
    static inline uint8_t match_mask8(uint64_t a, uint64_t b, uint64_t x, uint64_t y) 
    {
        uint64_t m = match_mask64(a, b, x, y);
        uint64_t bits = (m >> 7) & 0x0101010101010101ULL;
        return (uint8_t)((bits * 0x0102040810204080ULL) >> 56);
    }
    static inline uint64_t load_u64(const void* p)
    {
        uint64_t v;
        std::memcpy(&v, p, sizeof(v));
        return v;
    }
};

void* fast_memmem_SWAR( const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len)
{
    using namespace fm_SWAR;

    if(needle_len == 0)
        return (void *)haystack;
    if(needle_len > haystack_len)
        return nullptr;
    if(needle_len == 1)
        return (void *)memchr(haystack, ((const uint8_t *)needle)[0], haystack_len);

    uint64_t ffmask = (uint64_t)(((const uint8_t *)needle)[0]);
    ffmask |= (ffmask << 8);
    ffmask |= (ffmask << 16);
    ffmask |= (ffmask << 32);

    uint64_t ssmask = (uint64_t)(((const uint8_t *)needle)[needle_len - 1]);
    ssmask |= (ssmask << 8);
    ssmask |= (ssmask << 16);
    ssmask |= (ssmask << 32);

    const uint64_t fmask = ffmask;
    const uint64_t smask = ssmask;

    size_t i = 0;
    const uint8_t *hay_pos = (const uint8_t* )haystack;
    for(; i + needle_len + 7 <= haystack_len; i += 8, hay_pos += 8)
    {
        uint64_t amask = load_u64(hay_pos);
        uint64_t bmask = load_u64(hay_pos + needle_len - 1);

        uint8_t mmask = match_mask8(amask, bmask, fmask, smask);

        while(mmask)
        {
            int off = std::countr_zero(mmask);
            if(memcmp(needle, hay_pos + off, needle_len) == 0)
                return (void *)(hay_pos + off);
            mmask &= (uint8_t)(mmask - 1);
        }
    }
    for(; i + needle_len <= haystack_len; i ++, hay_pos += 1)
        if(memcmp(needle, hay_pos, needle_len) == 0)
            return (void *)hay_pos;

    return nullptr;
}

void* fast_memmem_SSE(  const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len)
{
    if(needle_len == 0)
        return (void *)haystack;
    if(needle_len > haystack_len)
        return nullptr;
    if(needle_len == 1)
        return (void *)memchr(haystack, ((const uint8_t *)needle)[0], haystack_len);

    const __m128i fmask = _mm_set1_epi8(((const uint8_t *)needle)[0]);
    const __m128i smask = _mm_set1_epi8(((const uint8_t *)needle)[needle_len - 1]);

    size_t i = 0;
    const uint8_t *hay_pos = (const uint8_t *)haystack;
    for(; i + 15 + needle_len <= haystack_len; i += 16, hay_pos += 16)
    {
        __m128i amask = _mm_loadu_si128((const __m128i *)hay_pos);
        __m128i bmask = _mm_loadu_si128((const __m128i *)(hay_pos + needle_len - 1));

        uint16_t mmask = _mm_movemask_epi8(_mm_and_si128(_mm_cmpeq_epi8(amask, fmask), _mm_cmpeq_epi8(bmask, smask)));

        while(mmask)
        {
            int off = std::countr_zero(mmask);
            if(memcmp(needle, hay_pos + off, needle_len) == 0)
                return (void *)(hay_pos + off);
            mmask &= (uint16_t)(mmask - 1);
        }
    }
    for(; i + needle_len <= haystack_len; i ++, hay_pos += 1)
        if(memcmp(needle, hay_pos, needle_len) == 0)
            return (void *)hay_pos;

    return nullptr; 
}

void* fast_memmem_AVX(  const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len)
{
    if(needle_len == 0)
        return (void *)haystack;
    if(needle_len > haystack_len)
        return nullptr;
    if(needle_len == 1)
        return (void *)memchr(haystack, ((const uint8_t *)needle)[0], haystack_len);

    const __m256i fmask = _mm256_set1_epi8(((const uint8_t *)needle)[0]);
    const __m256i smask = _mm256_set1_epi8(((const uint8_t *)needle)[needle_len - 1]);

    size_t i = 0;
    const uint8_t *hay_pos = (const uint8_t *)haystack;
    for(; i + 31 + needle_len <= haystack_len; i += 32, hay_pos += 32)
    {
        __m256i amask = _mm256_loadu_si256((const __m256i *)hay_pos);
        __m256i bmask = _mm256_loadu_si256((const __m256i *)(hay_pos + needle_len - 1));

        uint32_t mmask = _mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi8(amask, fmask), _mm256_cmpeq_epi8(bmask, smask)));

        while(mmask)
        {
            int off = std::countr_zero(mmask);
            if(memcmp(needle, hay_pos + off, needle_len) == 0)
                return (void *)(hay_pos + off);
            mmask &= (uint32_t)(mmask - 1);
        }
    }
    for(; i + needle_len <= haystack_len; i ++, hay_pos += 1)
        if(memcmp(needle, hay_pos, needle_len) == 0)
            return (void *)hay_pos;

    return nullptr; 
}