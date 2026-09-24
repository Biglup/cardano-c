#!/bin/bash

# Runs the unit tests that cover the files changed on the current branch under
# valgrind.
#
# Usage:
#   scripts/run-valgrind-on-changes.sh [base-ref]
#
# base-ref is the ref the current branch is compared against. It defaults to
# origin/main when that ref exists locally and to main otherwise, so the script
# works without network access. Set CARDANO_VALGRIND_FETCH=1 to run
# `git fetch origin main` before comparing; the script never fetches by default.
# Set CARDANO_TEST_BINARY=/path/to/test-cardano-c to choose the test binary;
# otherwise the most recently modified test-cardano-c under the current
# directory is used.
#
# The changed files are the files that differ from the merge base with base-ref
# plus the untracked files that git does not ignore, so a new test file is
# picked up before it is committed. Deleted files are skipped and renamed or
# copied files are taken at their new path. Each changed file is mapped to the
# unit test file under lib/tests with the same base name, and the exact
# suite.test pairs declared there with TEST( or TEST_F( form the gtest filter
# that runs under valgrind. When nothing changed, or when no changed file maps
# to a test file, the whole test suite runs.
#
# Run it from the repository root after building the debug tree.

if [ "${CARDANO_VALGRIND_FETCH:-0}" = "1" ]; then
    # Refresh the main branch so the comparison uses the latest remote state
    git fetch origin main
fi

if [ -n "$1" ]; then
    base_ref="$1"
elif git rev-parse --verify --quiet origin/main > /dev/null; then
    base_ref="origin/main"
else
    base_ref="main"
fi

merge_base=$(git merge-base "$base_ref" HEAD)

if [ -z "$merge_base" ]; then
    echo "Could not find a merge base between $base_ref and HEAD."
    exit 1
fi

echo "Comparing against $base_ref ($merge_base)"

changed_files=$(git diff --name-status "$merge_base" | awk -F '\t' '$1 != "D" {print $NF}') # Exclude deleted files, keep the new path of renames
untracked_files=$(git ls-files --others --exclude-standard) # New files that are not committed yet
changed_files=$(printf '%s\n%s\n' "$changed_files" "$untracked_files" | sed '/^$/d')

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

# Collect the suite.test pairs declared with TEST( or TEST_F( in the unit test files
test_names=""
for test_file in $unit_test_files; do
    test_names+="$(sed -nE 's/^[[:space:]]*TEST(_F)?\([[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*,[[:space:]]*([A-Za-z_][A-Za-z0-9_]*)[[:space:]]*\).*/\2.\3/p' "$test_file")"$'\n'
done

# Deduplicate test names
unique_test_names=$(echo "$test_names" | sed '/^$/d' | sort | uniq)

echo "Tests to run:"
echo "$unique_test_names"

# Locate the test binary, preferring the override and then the newest build
if [ -n "$CARDANO_TEST_BINARY" ]; then
    test_binary="$CARDANO_TEST_BINARY"
    if [ ! -f "$test_binary" ]; then
        echo "Test binary $test_binary not found."
        exit 1
    fi
else
    test_binary=$(find . -type f -name "test-cardano-c" -not -path "*CMakeFiles*" -printf '%T@ %p\n' | sort -n -r | head -n 1 | cut -d ' ' -f 2-)
    if [ -z "$test_binary" ]; then
        echo "Test binary not found."
        exit 1
    fi
fi

echo "Using test binary $test_binary"

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
elif [ -n "$changed_files" ]; then
    echo "No changed file maps to a unit test file. Running all tests."
    run_chunk "*"
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