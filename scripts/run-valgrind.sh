#!/bin/bash

script_folder="$(dirname $(readlink -f $0))"

ctest -T memcheck

# Count lines for memory leaks
error_count=`cat ./Testing/Temporary/MemoryChecker.*.log | wc -l`

if [ $error_count -gt 0 ]; then
  cat ./Testing/Temporary/MemoryChecker.*.log
fi

exit $error_count