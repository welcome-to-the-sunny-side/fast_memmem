// #define _GNU_SOURCE

#include <cstdio>
#include <cstdlib>
#include <utility>
#include <chrono>
#include <cstdint>
#include <string>
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


using memmem_fn = void* ( *)(const void *, const size_t, const void *, const size_t);

#include "fast_memmem.hpp"
memmem_fn our_memmem = fast_memmem_AVX;

memmem_fn enemy_memmem = memmem;

// #include "W_Mula_SWAR.hpp"
// memmem_fn enemy_memmem = swar64_strstr_anysize;

__attribute__((noinline, noipa))
void* call_once(memmem_fn fn, const void* h, const size_t hl, const void *n, const size_t nl)
{
    return fn(h, hl, n, nl);
};

const int PATH_LENGTH = 512;
const int TEST_LABEL = 100;
const int WARMUP = 10;
const int64_t CALIBRATION_TIMES_IN_MS = 10;
const int64_t MS_TO_NS = 1'000'000;
const int64_t MIN_BATCH_TIME_IN_MS = 100;

volatile uintptr_t trash_can = 0;

std::pair<char*, size_t> map_file(const char* path)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0)
        return {nullptr, 0};

    struct stat st;
    if(fstat(fd, &st) < 0)
    {
        close(fd);
        return {nullptr, 0};
    }

    size_t size = (size_t)st.st_size;
    void *ptr = mmap(nullptr, size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
    close(fd);

    if(ptr == MAP_FAILED)   return {nullptr, 0};
    return {(char*) ptr, size};
}

int64_t calibrate_batch_size (const char* haystack, const size_t haystack_len, const char* needle, const size_t needle_len)
{
    int64_t our_time = std::numeric_limits<int64_t>::max();
    for(int64_t n = 1; ; n *= 2)
    {
        auto t0 = std::chrono::steady_clock::now();
        for(int64_t i = 0; i < n; i ++)
            trash_can ^= (uintptr_t)call_once(our_memmem, haystack, haystack_len, needle, needle_len);
        auto t1 = std::chrono::steady_clock::now();
        
        int64_t elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds> (t1 - t0).count();

        if(elapsed_time < CALIBRATION_TIMES_IN_MS * MS_TO_NS)
            continue;

        //assume enough ns for floor div to not matter, 
        our_time = elapsed_time / n;
        break;
    }

    int64_t enemy_time = std::numeric_limits<int64_t>::max();

    for(int64_t n = 1; ; n *= 2)
    {
        auto t0 = std::chrono::steady_clock::now();
        for(int64_t i = 0; i < n; i ++)
            trash_can ^= (uintptr_t)call_once(enemy_memmem, haystack, haystack_len, needle, needle_len);        
        auto t1 = std::chrono::steady_clock::now();
        
        int64_t elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds> (t1 - t0).count();

        if(elapsed_time < CALIBRATION_TIMES_IN_MS * MS_TO_NS)
            continue;

        //assume enough ns for floor div to not matter, 
        enemy_time = elapsed_time / n;
        break;
    }

    int64_t bigger = std::max(our_time, enemy_time);
    if(bigger == int64_t(0))
        bigger = 1;

    int64_t batch_size = (MIN_BATCH_TIME_IN_MS * MS_TO_NS + bigger - 1) / bigger;

    return batch_size;
}

int main(int argc, char **argv)
{
    const char* test_dir = "../tests";
    if(argc > 2)        //expect --tests <testpath>
        test_dir = argv[2];
        
    for(int T = 0; ; T++)
    {
        char meta_path[PATH_LENGTH], hay_path[PATH_LENGTH], needle_path[PATH_LENGTH];
        snprintf(meta_path,   sizeof(meta_path),   "%s/test%d_metadata.inp", test_dir, T);
        snprintf(hay_path,    sizeof(hay_path),    "%s/test%d_haystack.inp", test_dir, T);
        snprintf(needle_path, sizeof(needle_path), "%s/test%d_needle.inp",   test_dir, T);

        FILE* meta = fopen(meta_path, "r");
        if (!meta) break;      // No more tests

        char test_label[TEST_LABEL];

        if(!fgets(test_label, sizeof(test_label), meta))
        {
            fclose(meta);
            break;
        }
        //stupid newline
        test_label[strcspn(test_label, "\n")] = '\0';

        int runs;
        fscanf(meta, "%d", &runs);
        fclose(meta);

        auto [haystack_ptr, haystack_size] = map_file(hay_path);
        auto [needle_ptr, needle_size] = map_file(needle_path);

        int64_t batch_size = calibrate_batch_size(haystack_ptr, haystack_size, needle_ptr, needle_size);

        std::printf("\n\nRunning test %d, labelled \"%s\", with (runs = %d) * (batch size = %ld) = %ld individual calls.\nHaystack size : %zu bytes \nNeedle size : %zu bytes\n", T, test_label, runs, batch_size, int64_t(runs) * int64_t(batch_size), haystack_size, needle_size);

        //small correctness check
        if(call_once(our_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size) != call_once(enemy_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size))
        {
            std::printf("Output mismatch on test %d. Aborting...\n", T);
            exit(0);
        }

        //us
        for(int j = 0; j < WARMUP; j ++)
            trash_can ^= (uintptr_t)call_once(our_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size);

        int64_t our_times[runs];

        for(int j = 0; j < runs; j ++)
        {
            auto t0 = std::chrono::steady_clock::now();
            for(int64_t k = 0; k < batch_size; k ++)
                trash_can ^= (uintptr_t)call_once(our_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size);
            auto t1 = std::chrono::steady_clock::now();
            our_times[j] = std::chrono::duration_cast<std::chrono::nanoseconds> (t1 - t0).count();
        }

        std::sort(our_times, our_times + runs);
        
        //enemy
        for(int j = 0; j < WARMUP; j ++)
            trash_can ^= (uintptr_t)call_once(enemy_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size);

        int64_t enemy_times[runs]; 

        for(int j = 0; j < runs; j ++)
        {
            auto t0 = std::chrono::steady_clock::now();
            for(int64_t k = 0; k < batch_size; k ++)
                trash_can ^= (uintptr_t)call_once(enemy_memmem, haystack_ptr, haystack_size, needle_ptr, needle_size);
            auto t1 = std::chrono::steady_clock::now();
            enemy_times[j] = std::chrono::duration_cast<std::chrono::nanoseconds> (t1 - t0).count();
        }

        std::sort(enemy_times, enemy_times + runs);

        std::printf("us vs enemy:\n\tat best : %.4f\n\tmedian: %.4f\n", (double)our_times[0]/(double)enemy_times[0], (double)our_times[runs/2]/(double)enemy_times[runs/2]);
        std::printf("our performance :\n\tmax GB/s : %.4f GB/s\n\tmedian GB/s : %.4f GB/s\n\tmin runtime : %.4f ns\n\tmedian runtime : %.4f ns\n", (double) batch_size * (double)haystack_size/(double)our_times[0], (double) batch_size * (double)haystack_size/(double)our_times[runs/2], (double)our_times[0]/(double) batch_size, (double)our_times[runs/2]/(double) batch_size);
        std::printf("enemy performance :\n\tmax GB/s : %.4f GB/s\n\tmedian GB/s : %.4f GB/s\n\tmin runtime : %.4f ns\n\tmedian runtime : %.4f ns\n", (double) batch_size * (double)haystack_size/(double)enemy_times[0], (double) batch_size * (double)haystack_size/(double)enemy_times[runs/2], (double)enemy_times[0]/(double) batch_size, (double)enemy_times[runs/2]/(double) batch_size);

        //cleanup
        munmap(needle_ptr, needle_size);
        munmap(haystack_ptr, haystack_size);
    }

    std::printf("%p\n", trash_can);
    return 0;
}