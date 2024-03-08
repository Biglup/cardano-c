#!/bin/bash

script_folder="$(dirname $(readlink -f $0))"

if [ "$#" -eq 1 ]; then
    test_name_regex="$1"
    echo "Running memcheck for tests matching: $test_name_regex"
    ctest -R "$test_name_regex" -T memcheck
else
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