#include <bit>
#include <cstdint>
#include <cstring>
#include <cstddef>
#include "fast_memmem.hpp"

#define SWAR

#ifdef SWAR
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
#endif

void* fast_memmem(  const void* haystack, const size_t haystack_len,
                    const void* needle, const size_t needle_len)
{
    if(needle_len == 0)
        return (void *)haystack;
    if(needle_len > haystack_len)
        return nullptr;

    //implement fast path later
    // if(needle_len == 1) {}

#ifdef SWAR
    const uint64_t *hay = (const uint64_t *)haystack;
    const uint64_t *ned = (const uint64_t *)needle;

    const uint64_t ffmask = (uint64_t)(((uint8_t *)needle)[0]);
    const uint64_t ssmask = (uint64_t)(((uint8_t *)needle)[needle_len - 1]);

    const uint64_t fmask = (ffmask) | (ffmask << 8) | (ffmask << 16) | (ffmask << 24) | (ffmask << 32) | (ffmask << 40) | (ffmask << 48) | (ffmask << 56);
    const uint64_t smask = (ssmask) | (ssmask << 8) | (ssmask << 16) | (ssmask << 24) | (ssmask << 32) | (ssmask << 40) | (ssmask << 48) | (ssmask << 56);

    size_t i = 0;
    uint8_t *hay_pos = (uint8_t* ) haystack;
    for(; i + needle_len + 7 <= haystack_len; i += 8, hay_pos += 8)
    {
        uint64_t amask = load_u64(((uint8_t *)haystack) + i);
        uint64_t bmask = load_u64(((uint8_t *)haystack) + i + needle_len - 1);

        uint8_t mmask = match_mask8(amask, bmask, fmask, smask);

        while(mmask)
        {
            int off = std::countr_zero(mmask);
            if(memcmp(needle, hay_pos + off, needle_len) == 0)
                return hay_pos + off;
            mmask &= (uint8_t)(mmask - 1);
        }
    }
    for(; i + needle_len <= haystack_len; i ++, hay_pos += 1)
        if(memcmp(needle, hay_pos, needle_len) == 0)
            return hay_pos;
#endif

    return nullptr;
}