#!/bin/bash

script_folder="$(dirname $(readlink -f $0))"

# Initialize variables with defaults
source_folder="$script_folder/../../lib"    # -s, --source
out_folder="$script_folder/.results"        # -o, --out
cppcheck_path="/usr/bin"                    # -c, --cppcheck
quiet=0                                     # -q, --quiet
output_xml=0                                # -x, --xml

function parse_command_line() {
   while [ $# -gt 0 ] ; do
    case "$1" in
      -s | --source) source_folder="$2" ;;
      -o | --out) out_folder="$2" ;;
      -c | --cppcheck) cppcheck_path="$2" ;;
      -q | --quiet) quiet=1 ;;
      -x | --xml) output_xml=1 ;;
      -*)
        echo "Unknown option: " $1
        exit 1
        ;;
    esac
    shift
  done
}

parse_command_line "$@"

cppcheck_bin="${cppcheck_path}/cppcheck"
cppcheck_misra="${cppcheck_path}/addons/misra.py"

num_cores=`getconf _NPROCESSORS_ONLN`
let num_cores--

mkdir -p "$out_folder"

cppcheck_parameters=( --inline-suppr
                      --quiet
                      --language=c
                      --addon="$script_folder/misra.json"
                      --suppressions-list="$script_folder/suppressions"
                      --cppcheck-build-dir="$out_folder"
                      "$source_folder/src")

cppcheck_out_file="$out_folder/results"
if [ $output_xml -eq 1 ]; then
  cppcheck_out_file="$out_folder/results.xml"
  cppcheck_parameters+=(--xml)
fi

"$cppcheck_bin" ${cppcheck_parameters[@]} 2> $cppcheck_out_file

# Count lines for broken rules
error_count=`grep '\[misra-c2012' < "$cppcheck_out_file" | wc -l`

if [ $quiet -eq 0 ]; then
  cat "$cppcheck_out_file"
fi
echo $error_count MISRA violations
echo $error_count > "$out_folder/error_count"

exit $error_count