#!/bin/bash

set -e

CPU=0
YIELDS=100000
RUNS=50
WARMUP_RUNS=5

TIMESTAMP=$(date +"%Y-%m-%d_%H-%M-%S")

RESULT_DIR="benchmark_results/${TIMESTAMP}"

mkdir -p "$RESULT_DIR"

CSV="${RESULT_DIR}/benchmark.csv"

echo "tasks,run,elapsed_ms,avg_ns,throughput" > "$CSV"

echo "======================================"
echo "Scheduler Scalability Benchmark"
echo "CPU          : $CPU"
echo "Yields/task  : $YIELDS"
echo "Runs/config  : $RUNS"
echo "Warm-up runs : $WARMUP_RUNS"
echo "Task counts  : 2 4 8 16 32 64"
echo "Result Dir   : $RESULT_DIR"
echo "======================================"
echo

for tasks in 2 4 8 16 32 64
do
    echo "Running ${tasks} tasks..."
    echo "Warm-up (${WARMUP_RUNS} runs)..."

    for warmup in $(seq 1 "$WARMUP_RUNS")
    do
        taskset -c "$CPU" \
            ./scheduler_benchmark \
            "$tasks" \
            "$YIELDS" \
            > /dev/null
    done

    echo "Warm-up completed."
    echo

    for run in $(seq 1 "$RUNS")
    do
        output=$(
            taskset -c "$CPU" \
                ./scheduler_benchmark \
                "$tasks" \
                "$YIELDS"
        )

        elapsed=$(
            echo "$output" |
                awk '/Elapsed time/ {print $4}'
        )

        avg=$(
            echo "$output" |
                awk '/Average yield cycle cost/ {print $6}'
        )

        throughput=$(
            echo "$output" |
                awk '/Yield throughput/ {print $4}'
        )

        if [[ -z "$elapsed" || -z "$avg" || -z "$throughput" ]]
        then
            echo "Error: failed to parse benchmark output"
            echo "Tasks: $tasks"
            echo "Run  : $run"
            echo
            echo "$output"
            exit 1
        fi

        echo "$tasks,$run,$elapsed,$avg,$throughput" >> "$CSV"

        if (( run % 10 == 0 ))
        then
            echo "  Completed $run / $RUNS runs"
        fi
    done

    echo "Finished ${tasks} tasks."
    echo
done

echo
echo "Raw benchmark finished."
echo

if [[ ! -f summarize.py ]]
then
    echo "Error: summarize.py not found in project root."
    exit 1
fi

cp summarize.py "$RESULT_DIR/"

(
    cd "$RESULT_DIR"
    python3 summarize.py
)

echo
echo "======================================"
echo "Benchmark Completed"
echo "======================================"
echo
echo "Results"
echo
echo "  Raw Data"
echo "    $CSV"
echo
echo "  Summary"
echo "    $RESULT_DIR/benchmark_summary.csv"
echo
echo "  Figures"
echo "    $RESULT_DIR/benchmark_figures/"
echo