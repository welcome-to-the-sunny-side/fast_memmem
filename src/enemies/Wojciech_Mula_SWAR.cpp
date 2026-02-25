#include <cstdint>
#include <cstddef>
#include <cstring>

void *swar64_strstr_anysize(const void* s_, const size_t n, const void* needle_, const size_t k) 
{
    const char *s = (const char *)s_;
    const char *needle = (const char *)needle_;

    const uint64_t first = 0x0101010101010101llu * static_cast<uint8_t>(needle[0]);
    const uint64_t last  = 0x0101010101010101llu * static_cast<uint8_t>(needle[k - 1]);

    uint64_t* block_first = reinterpret_cast<uint64_t*>(const_cast<char*>(s));
    uint64_t* block_last  = reinterpret_cast<uint64_t*>(const_cast<char*>(s + k - 1));

    for (auto i=0u; i < n; i+=8, block_first++, block_last++) {
        const uint64_t eq = (*block_first ^ first) | (*block_last ^ last);

        const uint64_t t0 = (~eq & 0x7f7f7f7f7f7f7f7fllu) + 0x0101010101010101llu;
        const uint64_t t1 = (~eq & 0x8080808080808080llu);
        uint64_t zeros = t0 & t1;
        size_t j = 0;

        while (zeros) {
            if (zeros & 0x80) {
                const char* substr = reinterpret_cast<char*>(block_first) + j + 1;
                if (memcmp(substr, needle + 1, k - 2) == 0) {
                    return (void*)(s + (i + j));
                }
            }

            zeros >>= 8;
            j += 1;
        }
    }

    return nullptr;
}