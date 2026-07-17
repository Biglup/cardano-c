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

# Locate the test binary produced by the build
test_binary=$(find . -type f -name "test-cardano-c" -not -path "*CMakeFiles*" | head -n 1)

if [ -z "$test_binary" ]; then
    echo "Test binary not found."
    exit 1
fi

# Run the selected tests under a single valgrind process per chunk. Running the
# gtest binary directly (instead of one ctest memcheck process per test) pays the
# valgrind startup cost once per chunk rather than once per test, which keeps
# large diffs feasible.
valgrind_cmd="valgrind -q --error-exitcode=1 --leak-check=full --track-origins=no"
chunk_size=400
error_count=0

run_chunk() {
    filter="$1"
    if ! $valgrind_cmd "$test_binary" --gtest_filter="$filter" --gtest_brief=1; then
        error_count=$((error_count + 1))
    fi
}

if [ -n "$unique_test_names" ]; then
    echo "Running tests for changed files: $unit_test_files"
    filter=""
    count=0
    while IFS= read -r test_name; do
        [ -z "$test_name" ] && continue
        if [ -n "$filter" ]; then
            filter="$filter:$test_name"
        else
            filter="$test_name"
        fi
        count=$((count + 1))
        if [ "$count" -ge "$chunk_size" ]; then
            run_chunk "$filter"
            filter=""
            count=0
        fi
    done <<< "$unique_test_names"
    if [ -n "$filter" ]; then
        run_chunk "$filter"
    fi
else
    echo "No changes detected. Running all tests."
    run_chunk "*"
fi

if [ "$error_count" -gt 0 ]; then
  echo "Memory errors detected in $error_count chunk(s), see the valgrind output above."
else
  echo "No memory leaks detected."
fi

exit $error_count