#!/bin/bash

# Check if both parameters were provided
if [ $# -ne 2 ]; then
    echo "Usage: $0 <number_of_files> <values_per_file>"
    echo "Example: $0 11 1024"
    echo "This will generate 11 test files, each containing 1024 values"
    exit 1
fi

# Get parameters from command line arguments
NUM_FILES=$1
NUM_VALUES=$2

# Validate inputs are positive numbers
if ! [[ "$NUM_FILES" =~ ^[0-9]+$ ]] || [ "$NUM_FILES" -lt 1 ]; then
    echo "Error: Please provide a positive number for number of files"
    exit 1
fi

if ! [[ "$NUM_VALUES" =~ ^[0-9]+$ ]] || [ "$NUM_VALUES" -lt 1 ]; then
    echo "Error: Please provide a positive number for values per file"
    exit 1
fi

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

# Clean up previous test data
echo "Cleaning up previous test data..."
rm -f "$SCRIPT_DIR/tools/sealed_data/"*.dat
rm -f "$SCRIPT_DIR/aes.key"
echo "Previous test data cleaned up"

# Create directory if it doesn't exist
mkdir -p "$SCRIPT_DIR/tools/sealed_data"

# Maximum value (2^23 - 1)
MAX_VAL=$((2**23-1))

echo "Generating $NUM_FILES files with $NUM_VALUES values each..."

# Generate a single AES key first that will be used for all files
echo "Generating master AES key..."
echo "0" > "$SCRIPT_DIR/init.txt"
"$SCRIPT_DIR/tools/value_sealer/value_sealer" seal-tests \
    "$SCRIPT_DIR/tools/sealed_data/init.dat" \
    "$SCRIPT_DIR/init.txt"
rm "$SCRIPT_DIR/init.txt" "$SCRIPT_DIR/tools/sealed_data/init.dat"

# Store the original key
mv "$SCRIPT_DIR/aes.key" "$SCRIPT_DIR/master.key"

# Generate and seal multiple files
for i in $(seq 1 $NUM_FILES); do
    echo "Generating and sealing file set $i of $NUM_FILES..."

    # Generate secret numbers
    echo "Generating $NUM_VALUES secret numbers..."
    shuf -i 0-$MAX_VAL -n $NUM_VALUES | sort -n > "$SCRIPT_DIR/secret_numbers${i}.txt"

    # Generate test numbers
    echo "-$i" > "$SCRIPT_DIR/test_numbers${i}.txt"

    # Restore the master key before each sealing operation
    cp "$SCRIPT_DIR/master.key" "$SCRIPT_DIR/aes.key"

    # Seal the files
    echo "Sealing files..."
    "$SCRIPT_DIR/tools/value_sealer/value_sealer" seal \
        "$SCRIPT_DIR/tools/sealed_data/secret_numbers${i}.dat" \
        "$SCRIPT_DIR/secret_numbers${i}.txt"

    "$SCRIPT_DIR/tools/value_sealer/value_sealer" seal-tests \
        "$SCRIPT_DIR/tools/sealed_data/test_numbers${i}.dat" \
        "$SCRIPT_DIR/test_numbers${i}.txt"

    # Clean up temporary files
    rm "$SCRIPT_DIR/secret_numbers${i}.txt" "$SCRIPT_DIR/test_numbers${i}.txt"

    echo "Completed file set $i"
    echo "-------------------"
done

# Restore the master key one final time
cp "$SCRIPT_DIR/master.key" "$SCRIPT_DIR/aes.key"
rm "$SCRIPT_DIR/master.key"

echo "All $NUM_FILES sets of files generated and sealed!"
