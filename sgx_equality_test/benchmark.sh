#!/bin/bash

# Exit on any error
set -e

# Function to print with timestamp
log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $1"
}

# Create results directory if it doesn't exist
mkdir -p results

# Get current timestamp for the results file
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
RESULTS_FILE="results/benchmark_${TIMESTAMP}.txt"
STATS_FILE="results/stats_${TIMESTAMP}.csv"

# Create CSV header
echo "Size,Min (μs),Mean (μs),Median (μs),Max (μs),Operations/s" > "$STATS_FILE"

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

    # Generate test files for current size (25 files)
    log "Generating 25 test files..."
    ./generate_test_files.sh 25 $size

    # Run the test and save output to temporary file
    log "Running equality test..."
    ./sgx_equality_test 25 | tee -a "$RESULTS_FILE" > temp_output.txt

    # Extract statistics from the temporary file
    min=$(grep "Minimum:" temp_output.txt | awk '{print $2}')
    mean=$(grep "Mean:" temp_output.txt | awk '{print $2}')
    median=$(grep "Median:" temp_output.txt | awk '{print $2}')
    max=$(grep "Maximum:" temp_output.txt | awk '{print $2}')
    ops=$(grep "Average operations per second:" temp_output.txt | awk '{print $5}')

    # Write to CSV
    echo "$size,$min,$mean,$median,$max,$ops" >> "$STATS_FILE"

    # Remove temporary file
    rm temp_output.txt

    # Add separator in results file
    echo -e "\n----------------------------------------\n" >> "$RESULTS_FILE"

    # Clean up sealed data files
    rm -f ./tools/sealed_data/*.dat

    # Small delay between tests
    sleep 2
done

log "Benchmark completed. Results saved to:"
log "  - Full results: $RESULTS_FILE"
log "  - Statistics: $STATS_FILE"

# Print summary of results
log "Summary of results:"
echo "----------------------------------------"
cat "$STATS_FILE"
echo "----------------------------------------"
