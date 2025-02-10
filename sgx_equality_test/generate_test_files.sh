#!/bin/bash

# Check if number of files was provided as argument
if [ $# -ne 1 ]; then
    echo "Usage: $0 <number_of_files>"
    echo "Example: $0 20"
    exit 1
fi

# Get number of files from command line argument
NUM_FILES=$1

# Validate input is a positive number
if ! [[ "$NUM_FILES" =~ ^[0-9]+$ ]] || [ "$NUM_FILES" -lt 1 ]; then
    echo "Error: Please provide a positive number"
    exit 1
fi

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Check if required directories exist
if [ ! -d "$SCRIPT_DIR/tools/value_sealer" ]; then
    echo "Error: tools/value_sealer directory not found"
    echo "Please run this script from the project root directory"
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/tools/value_sealer/value_sealer" ]; then
    echo "Error: value_sealer executable not found"
    echo "Please compile value_sealer first"
    exit 1
fi

# Create directory if it doesn't exist
mkdir -p "$SCRIPT_DIR/tools/sealed_data"

# Maximum value (2^23 - 1)
MAX_VAL=$((2**23-1))

# Number of secret values (2^22)
NUM_SECRETS=$((2**22))

echo "Generating $NUM_FILES sets of files..."

# Generate and seal multiple files
for i in $(seq 1 $NUM_FILES); do
    echo "Generating and sealing file set $i of $NUM_FILES..."

    # Generate secret numbers
    echo "Generating secret numbers..."
    shuf -i 0-$MAX_VAL -n $NUM_SECRETS | sort -n > "$SCRIPT_DIR/secret_numbers${i}.txt"

    # Generate different test number for each file (negative numbers)
    echo "-$i" > "$SCRIPT_DIR/test_numbers${i}.txt"

    # Seal the files
    echo "Sealing files..."

    # Use absolute paths for value_sealer
    "$SCRIPT_DIR/tools/value_sealer/value_sealer" seal \
        "$SCRIPT_DIR/tools/sealed_data/secret_numbers${i}.dat" \
        "$SCRIPT_DIR/secret_numbers${i}.txt"

    "$SCRIPT_DIR/tools/value_sealer/value_sealer" seal-tests \
        "$SCRIPT_DIR/tools/sealed_data/test_numbers${i}.dat" \
        "$SCRIPT_DIR/test_numbers${i}.txt"

    # Clean up temporary text files
    rm "$SCRIPT_DIR/secret_numbers${i}.txt" "$SCRIPT_DIR/test_numbers${i}.txt"

    echo "Completed file set $i"
    echo "-------------------"
done

echo "All $NUM_FILES sets of files generated and sealed!"
