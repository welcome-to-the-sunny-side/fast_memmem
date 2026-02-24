#pragma once

#include <cstddef>

void* fast_memmem(  const void* haystack, const size_t haystack_len,
                    const void* needle, const size_t needle_len);