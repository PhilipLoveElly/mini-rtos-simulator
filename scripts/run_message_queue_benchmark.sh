#!/bin/bash

set -e

CPU=0
MESSAGES=100000
WARMUP_RUNS=5
MEASURED_RUNS=50

TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

RESULT_DIR="benchmark_results/message_queue/${TIMESTAMP}"

mkdir -p "$RESULT_DIR"

CSV="${RESULT_DIR}/message_queue.csv"

echo "run,capacity,messages,elapsed_ns,throughput,producer_blocks,consumer_blocks,checksum" > "$CSV"

CAPACITIES=(
1
2
4
8
16
32
64
128
256
512
1024
2048
4096
8192
)

echo "========================================"
echo "Message Queue Benchmark"
echo "========================================"
echo "CPU           : $CPU"
echo "Messages      : $MESSAGES"
echo "Warm-up Runs  : $WARMUP_RUNS"
echo "Measured Runs : $MEASURED_RUNS"
echo "Result Dir    : $RESULT_DIR"
echo "========================================"
echo

for capacity in "${CAPACITIES[@]}"
do
    echo "Capacity = ${capacity}"

    # Warm-up runs: execute but do not save.
    for run in $(seq 1 "$WARMUP_RUNS")
    do
        taskset -c "$CPU" \
            ./message_queue_benchmark \
            "$capacity" \
            "$MESSAGES" \
            > /dev/null
    done

    # Measured runs.
    for run in $(seq 1 "$MEASURED_RUNS")
    do
        result=$(
            taskset -c "$CPU" \
                ./message_queue_benchmark \
                "$capacity" \
                "$MESSAGES"
        )

        echo "${run},${result}" >> "$CSV"
    done
done

echo
echo "========================================"
echo "Benchmark Complete"
echo "========================================"
echo "CSV saved to:"
echo "$CSV"