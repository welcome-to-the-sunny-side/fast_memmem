#!/bin/bash

# Usage:
#   ./generate.sh          # runs all generators found
#   ./generate.sh 0 5      # runs generators 0 through 5 (inclusive)
#   ./generate.sh 3 3      # runs just generator 3

set -e

if [ $# -eq 2 ]; then
    lo=$1
    hi=$2
else
    lo=0
    hi=-1  # sentinel: means "keep going until we run out"
fi

i=$lo
while true; do
    src="gen_test${i}.cpp"

    if [ ! -f "$src" ]; then
        if [ $hi -eq -1 ]; then
            break           # auto mode: stop at first missing generator
        else
            echo "Warning: $src not found, skipping"
            if [ $i -ge $hi ]; then break; fi
            i=$((i + 1))
            continue
        fi
    fi

    echo "=== Generating test $i ==="
    g++ -O2 -o "gen_test${i}" "$src"
    ./gen_test${i}
    rm -f "gen_test${i}"    # clean up binary

    if [ $hi -ne -1 ] && [ $i -ge $hi ]; then break; fi
    i=$((i + 1))
done

echo "Done."
