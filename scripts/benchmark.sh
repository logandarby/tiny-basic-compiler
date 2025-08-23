#!/bin/bash

# Number of runs
RUNS=10
TIMES=()

echo "Running benchmark $RUNS times..."

for i in $(seq 1 $RUNS); do
    echo "Run $i/$RUNS..."
    
    # Run make perf and capture the time output
    TIME_OUTPUT=$(make perf > /dev/null 2>&1 && /usr/bin/time -f "%e" ./builds/perf/teeny-perf --emit-asm ./examples/BENCHMARK.basic 2>&1 >/dev/null | tail -1)
    
    # Add the time to our array
    TIMES+=($TIME_OUTPUT)
    
    echo "  Time: ${TIME_OUTPUT}s"
done

echo ""
echo "Results:"
echo "========"

# Calculate average
TOTAL=0
for time in "${TIMES[@]}"; do
    TOTAL=$(echo "$TOTAL + $time" | bc -l)
done

AVERAGE=$(echo "scale=4; $TOTAL / $RUNS" | bc -l)

echo "Individual times: ${TIMES[*]}"
echo "Average time: ${AVERAGE}s"
echo "Total runs: $RUNS"