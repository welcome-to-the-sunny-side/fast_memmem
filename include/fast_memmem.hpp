#pragma once

#include <cstddef>

void* fast_memmem_SWAR( const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len);

void* fast_memmem_SSE(  const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len);

void* fast_memmem_AVX(  const void* haystack, const size_t haystack_len,
                        const void* needle, const size_t needle_len);