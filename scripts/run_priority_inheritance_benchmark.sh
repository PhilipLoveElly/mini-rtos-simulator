#!/bin/bash

set -e

RESULT_DIR="benchmark_results/priority_inheritance"

mkdir -p "$RESULT_DIR"

CSV="${RESULT_DIR}/priority_inheritance.csv"

echo "pi_enabled,medium_iterations,low_critical_iterations,request_tick,acquire_tick,wait_ticks" > "$CSV"

RUNS=20

ITERATIONS=(
0
10
25
50
100
250
500
1000
)

echo "========================================"
echo "Priority Inheritance Benchmark"
echo "Runs : $RUNS"
echo "========================================"

for iteration in "${ITERATIONS[@]}"
do
    echo
    echo "Medium iterations = ${iteration}"

    for run in $(seq 1 $RUNS)
    do
        ./priority_inheritance_benchmark 1 "$iteration" >> "$CSV"
        ./priority_inheritance_benchmark 0 "$iteration" >> "$CSV"
    done
done

echo
echo "Done."
echo "CSV saved to:"
echo "$CSV"