#!/bin/bash

# Check if a file path is provided as an argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 path_to_header_file.h"
    exit 1
fi

# Define the header file from the first argument
HEADER_FILE="$1"

# Check if the header file exists
if [ ! -f "$HEADER_FILE" ]; then
    echo "File not found: $HEADER_FILE"
    exit 1
fi

# Use an associative array to track seen functions
declare -A seen

# Extracting function names and generating the Sphinx directives
# Using sed to remove comments and then grep to find function declarations
sed '/\/\*/,/\*\//d' "$HEADER_FILE" | \
grep -oP '\bcardano_[a-zA-Z0-9_]+\s*\(' | \
sed 's/($//' | \
while read -r line
do
    func_name=$(echo "$line" | awk '{print $1}')
    if [[ "$func_name" =~ ^cardano_ ]] && [ -z "${seen[$func_name]}" ]; then
        seen[$func_name]=1
        echo ""
        echo "------------"
        echo ""
        echo ".. doxygenfunction:: $func_name"
    fi
done