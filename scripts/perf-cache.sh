#!/bin/bash

# Performance profiling script for cache and branch prediction analysis
# Usage: ./scripts/perf-cache.sh [input_file]

# Build performance binary
echo "Building performance binary..."
make perf

# Configuration
FILE="${1:-./examples/big_file.basic}"
CMD="./builds/perf/teeny-perf --emit-asm"

# Comprehensive event list for cache and branch analysis
CACHE_EVENTS="cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,L1-icache-loads,L1-icache-load-misses,LLC-loads,LLC-load-misses"
BRANCH_EVENTS="branches,branch-misses,branch-loads,branch-load-misses"
CPU_EVENTS="cycles,instructions,cpu-clock,task-clock"
MEMORY_EVENTS="page-faults,minor-faults,major-faults"

ALL_EVENTS="${CACHE_EVENTS},${BRANCH_EVENTS},${CPU_EVENTS},${MEMORY_EVENTS}"

echo "=============================================="
echo "Performance Analysis for: ${FILE}"
echo "Command: ${CMD} ${FILE}"
echo "=============================================="

# Run comprehensive performance statistics
echo ""
sudo perf stat -e ${ALL_EVENTS} ${CMD} ${FILE}

