#!/bin/bash

# Make sure we fetch the main branch so we can compare the changes
git fetch origin main

script_folder="$(dirname $(readlink -f $0))"
changed_files=$(git diff --name-status $(git merge-base origin/main HEAD) | awk '$1 != "D" {print $2}') # Exclude deleted files

# Extract file names without extensions
changed_files_base=$(echo "$changed_files" | sed 's/\(.*\)\..*/\1/' | sort | uniq)

# Find corresponding unit test files
unit_test_files=""
for file in $changed_files_base; do
    file_name=$(basename "$file")
    unit_test_file=$(find lib/tests -type f -wholename "**/${file_name}.cpp" | head -n 1)
    if [ -n "$unit_test_file" ]; then
        unit_test_files+=" $unit_test_file"
    fi
done

unit_test_files=$(echo "$unit_test_files" | tr ' ' '\n' | sort | uniq)

# Collect test names from unit test files
test_names=""
for test_file in $unit_test_files; do
    # Extract full test names and append to the test_names variable
    test_names+=" $(grep -oP 'TEST\(\K[^)]+' "$test_file" | tr -d ',' | awk '{ print $1"."$2 }')"
done

# Deduplicate test names
unique_test_names=$(echo "$test_names" | tr ' ' '\n' | sort | uniq)

echo "Tests to run:"
echo "$unique_test_names"

# Run tests for changed files if any
if [ -n "$unique_test_names" ]; then
    echo "Running tests for changed files: $unit_test_files"
    while IFS= read -r test_name; do
        ctest -R "$test_name" -T memcheck
    done <<< "$unique_test_names"
else
    echo "No changes detected. Running all tests."
    ctest -T memcheck
fi

# Count lines for memory leaks
error_count=$(cat ./Testing/Temporary/MemoryChecker.*.log | wc -l)

if [ "$error_count" -gt 0 ]; then
  echo "Memory leaks detected:"
  cat ./Testing/Temporary/MemoryChecker.*.log
else
  echo "No memory leaks detected."
fi

exit $error_count