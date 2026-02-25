#include <cstring>
#include <cstddef>
#include "fast_memmem.hpp"

// #define TRIVIAL

void* fast_memmem(  const void* haystack, const size_t haystack_len,
                    const void* needle, const size_t needle_len)
{
    const unsigned char *hay = (const unsigned char *)haystack;
    const unsigned char *ned = (const unsigned char *)needle;

    return nullptr;
}