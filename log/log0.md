ok just finished the harness, added in stupid repeated `memcmp`s just to test it out

```bash
non@fedora ~/P/f/tests (main)> ./generate.sh
=== Generating test 0 ===
Done.
non@fedora ~/P/f/tests (main)> cd ..
non@fedora ~/P/fast_memmem (main)> g++ src/driver.cpp
 -Iinclude include/fast_memmem.cpp -o bin/driver.bin
non@fedora ~/P/fast_memmem (main)> cd bin
non@fedora ~/P/f/bin (main)> ./driver.bin


Running test 0, labelled randomized (probably) single occurrence, with (runs = 10) * (batch size = 6150) = 61500 individual calls.
Haystack size : 1002435 bytes 
Needle size : 3544 bytes
us vs enemy:
        at best : 14.8391
        median: 17.6660
our performance :
        max GB/s : 3.2951 GB/s
        median GB/s : 2.7607 GB/s
        min runtime : 304220.9478 ns
        median runtime : 363114.4754 ns
enemy performance :
        max GB/s : 48.8962 GB/s
        median GB/s : 48.7697 GB/s
        min runtime : 20501.2849 ns
        median runtime : 20554.4707 ns
```