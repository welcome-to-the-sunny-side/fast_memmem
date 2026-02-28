#include <cstdio>
#include <cstdlib>
#include <random>
#include <ctime>
#include <cstring>
#include <cassert>
#include <fcntl.h>
#include <unistd.h>

const int TEST_ID = 2;
char TEST_LABEL[] = "3-char alphabet, randomized string, so memcmp gets called very frequently";
const int RUNS = 10;
const int ALPHA = 3;            //[1, 256)
const size_t HAY_SZ = 356356;
const size_t NED_SZ = 3544;
const int SEED = 354399634;

char base_path[512];
char meta_path[512];
char haystack_path[512];
char needle_path[512];

std::mt19937 rng(SEED);
std::uniform_int_distribution<int> dist(1, ALPHA);

void write_to_file(int fd, char *buf, size_t len)
{
    while(len > 0)
    {
        ssize_t written = write(fd, buf, len);
        assert(written >= 0 && "write failed");
        buf += written;
        len -= written;
    }
}

int main()
{
    static_assert(1 <= ALPHA and ALPHA < 256, "alphabet size is inappropriate");
    static_assert(NED_SZ <= HAY_SZ, "the needle shouldn't be bigger dumbass");

    snprintf(base_path, sizeof(base_path), "test%d", TEST_ID);
    snprintf(meta_path, sizeof(meta_path), "%s_metadata.inp", base_path);
    snprintf(haystack_path, sizeof(haystack_path), "%s_haystack.inp", base_path);
    snprintf(needle_path, sizeof(needle_path), "%s_needle.inp", base_path);

    int meta_fd = open(meta_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int hay_fd = open(haystack_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    int ned_fd = open(needle_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    assert(std::min(meta_fd, std::min(hay_fd, ned_fd)) >= 0 && "file error");

    //metadata
    char* metadata_buf = nullptr;
    size_t metadata_buf_sz = 1 + snprintf(metadata_buf, 0, "%s\n%d", TEST_LABEL, RUNS);
    metadata_buf = (char *)malloc(metadata_buf_sz);
    snprintf(metadata_buf, metadata_buf_sz, "%s\n%d", TEST_LABEL, RUNS);
    write_to_file(meta_fd, metadata_buf, metadata_buf_sz - 1);      //exclude the null-terminator
    free(metadata_buf);
    close(meta_fd);

    //needle and haystack
    uint8_t *needle = (uint8_t *)malloc(NED_SZ);
    for(size_t i = 0; i < NED_SZ; i ++)
        needle[i] = (uint8_t) dist(rng);

    uint8_t *haystack = (uint8_t *)malloc(HAY_SZ);
    size_t pos = std::uniform_int_distribution<size_t>(0, HAY_SZ - NED_SZ) (rng);
    for(size_t i = 0; i < HAY_SZ; i ++)
    {
        if(i >= pos and i < pos + NED_SZ)
            haystack[i] = needle[i - pos];
        else
            haystack[i] = (uint8_t) dist(rng);
    }

    write_to_file(ned_fd, (char *)needle, NED_SZ);
    write_to_file(hay_fd, (char *)haystack, HAY_SZ);
    
    close(ned_fd);
    close(hay_fd);

    free(needle);
    free(haystack);
}