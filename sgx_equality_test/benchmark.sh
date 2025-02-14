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
echo "Size,Avg_TotalTime_ms,Avg_ProcessingTime_ms,Avg_EncryptionTime_ms,Avg_OverheadTime_ms,Avg_PageFaults" > "$SUMMARY_FILE"

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
    echo "Test,Matches,NonMatches,Errors,TotalTime_ms,ProcessingTime_ms,EncryptionTime_ms,OverheadTime_ms,PageFaults" > "$DETAIL_FILE"

    # Run the test and save output
    log "Running equality test..."
    ./sgx_equality_test 50 | tee >(grep "^[0-9]" > temp_output.txt)

    # Convert microseconds to milliseconds and save to size-specific CSV
    awk -F',' '{printf "%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%d\n", 
        $1, $2, $3, $4, 
        $5/1000, $6/1000, $7/1000, $8/1000, 
        $9}' temp_output.txt > temp_converted.txt
    cat temp_converted.txt >> "$DETAIL_FILE"

    # Calculate averages for summary (already in milliseconds)
    awk -F',' '
    BEGIN {total=0; proc=0; enc=0; over=0; page=0; count=0}
    {
        total+=$5; proc+=$6; enc+=$7; over+=$8; page+=$9; count++
    }
    END {
        if(count>0) {
            printf "%d,%.3f,%.3f,%.3f,%.3f,%.2f\n", 
            '"$size"', 
            total/count, 
            proc/count, 
            enc/count, 
            over/count, 
            page/count
        }
    }' temp_converted.txt >> "$SUMMARY_FILE"

    # Remove temporary files
    rm temp_output.txt temp_converted.txt

    # Clean up sealed data files
    rm -f ./tools/sealed_data/*.dat

    # Small delay between tests
    sleep 2
done

log "Benchmark completed. Results saved to: $RESULTS_DIR"
log "Summary of results:"
echo "----------------------------------------"
cat "$SUMMARY_FILE"
echo "----------------------------------------"
