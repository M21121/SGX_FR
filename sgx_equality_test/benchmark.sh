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

    # Generate test files for current size (50 files)
    log "Generating 50 test files..."
    ./generate_test_files.sh 50 $size

    # Create detailed results file for this size
    DETAIL_FILE="${RESULTS_DIR}/size_${size}.csv"
    echo "Test,DecryptionTime_ms,IntersectionTime_ms,IntersectionResult,EncryptionTime_ms,TotalRuntime_ms" > "$DETAIL_FILE"

    # Run the test and capture output
    log "Running equality test..."
    ./sgx_equality_test 50 | while IFS=',' read -r test matches nonmatches errors totaltime processtime enctime dectime overhead pagefaults; do
        # Skip header line
        if [ "$test" != "Test" ]; then
            # Convert microseconds to milliseconds and calculate total runtime
            dec_ms=$(echo "scale=3; $dectime/1000" | bc)
            proc_ms=$(echo "scale=3; $processtime/1000" | bc)
            enc_ms=$(echo "scale=3; $enctime/1000" | bc)
            total_ms=$(echo "scale=3; $totaltime/1000" | bc)

            # Write to detail file
            echo "$test,$dec_ms,$proc_ms,$matches,$enc_ms,$total_ms" >> "$DETAIL_FILE"
        fi
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
