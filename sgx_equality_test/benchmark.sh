#!/bin/bash

# Exit on any error
set -e

# Function to print with timestamp
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Create results directory if it doesn't exist
mkdir -p results

# Get current timestamp for the results directory
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
RESULTS_DIR="results/benchmark_${TIMESTAMP}"
mkdir -p "$RESULTS_DIR"

# Create summary stats file
SUMMARY_FILE="${RESULTS_DIR}/summary.csv"
echo "Size,Avg_DecryptionTime_ms,Avg_IntersectionTime_ms,Avg_IntersectionResult,Avg_EncryptionTime_ms,Avg_TotalRuntime_ms" > "$SUMMARY_FILE"

# Clean and build the project
log "Cleaning previous build..."
make clean

log "Building project..."
make

# Initialize array of sizes (2^10 to 2^20)
SIZES=(1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)

# Run tests for each size
for size in "${SIZES[@]}"; do
    log "Running test with size: $size"

    # Create directory for this size if it doesn't exist
    SIZE_DIR="${RESULTS_DIR}/size_${size}"
    mkdir -p "$SIZE_DIR"

    # Create detailed results file for this size
    DETAIL_FILE="${SIZE_DIR}/results.csv"
    touch "$DETAIL_FILE"
    echo "Test,DecryptionTime_ms,IntersectionTime_ms,IntersectionResult,EncryptionTime_ms,TotalRuntime_ms" > "$DETAIL_FILE"

    # Run 50 iterations for each size
    for i in {1..50}; do
        log "Running iteration $i of 50..."

        # Generate test file with single value for this iteration
        log "Generating test file..."
        ./generate_test_files.sh 1 $size

        # Measure start time
        MAIN_START=$(date +%s.%N)

        # Run the test and capture output
        MAIN_OUTPUT=$(./sgx_equality_test 1 | tail -n 1)

        # Measure end time
        MAIN_END=$(date +%s.%N)

        # Calculate total time in milliseconds
        TOTAL_TIME=$(echo "$MAIN_END - $MAIN_START" | bc | awk '{printf "%.3f", $1 * 1000}')

        # Parse the original output for other metrics
        IFS=',' read -r test matches nonmatches errors totaltime processtime enctime dectime overhead pagefaults <<< "$MAIN_OUTPUT"

        # Convert other times from microseconds to milliseconds
        dec_ms=$(echo "scale=3; $dectime/1000" | bc)
        proc_ms=$(echo "scale=3; $processtime/1000" | bc)
        enc_ms=$(echo "scale=3; $enctime/1000" | bc)

        # Write to detail file with measured total time
        echo "$i,$dec_ms,$proc_ms,$matches,$enc_ms,$TOTAL_TIME" >> "$DETAIL_FILE"
    done

    # Calculate averages for summary
    log "Calculating averages..."
    avg_dec_ms=$(awk -F',' 'NR>1 {sum+=$2} END {printf "%.3f", sum/(NR-1)}' "$DETAIL_FILE")
    avg_proc_ms=$(awk -F',' 'NR>1 {sum+=$3} END {printf "%.3f", sum/(NR-1)}' "$DETAIL_FILE")
    avg_matches=$(awk -F',' 'NR>1 {sum+=$4} END {printf "%.3f", sum/(NR-1)}' "$DETAIL_FILE")
    avg_enc_ms=$(awk -F',' 'NR>1 {sum+=$5} END {printf "%.3f", sum/(NR-1)}' "$DETAIL_FILE")
    avg_total_ms=$(awk -F',' 'NR>1 {sum+=$6} END {printf "%.3f", sum/(NR-1)}' "$DETAIL_FILE")

    # Add to summary file
    echo "$size,$avg_dec_ms,$avg_proc_ms,$avg_matches,$avg_enc_ms,$avg_total_ms" >> "$SUMMARY_FILE"

    log "Completed test for size $size"
    log "----------------------------------------"
done

log "Benchmark complete! Results are in ${RESULTS_DIR}"
