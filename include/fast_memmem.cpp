#include <cstring>
#include <cstddef>
#include "fast_memmem.hpp"

#define TRIVIAL

void* fast_memmem(  const void* haystack, const size_t haystack_len,
                    const void* needle, const size_t needle_len)
{
    #ifdef TRIVIAL
        const unsigned char *hay = (const unsigned char *)haystack;
        const unsigned char *ned = (const unsigned char *)needle;
        for(size_t i = 0; i + needle_len <= haystack_len; i ++)
            if(memcmp(hay + i, ned, needle_len) == 0)
                return (void *)(hay + i);
    #endif

    return nullptr;
}