#!/bin/bash

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed."
    exit 1
fi

DIRECTORIES=(
    "lib"
    "src"
    "test"
    "benchmark"
)

# Loop through each directory provided
for DIRECTORY in "${DIRECTORIES[@]}"; do
    # Check if the directory exists
    if [ ! -d "$DIRECTORY" ]; then
        echo "Error: Directory '$DIRECTORY' does not exist."
        continue
    fi
    
    # Get a list of all C++ source files in the directory
    FILES=$(find "$DIRECTORY" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.h" \))

    # Loop through each file and format it using clang-format
    for FILE in $FILES; do
        echo "Formatting $FILE ..."
        clang-format -i $FILE
    done
done

echo "Formatting complete."