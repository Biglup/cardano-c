#!/bin/bash

# SETTINGS
# set path to clang-format binary
if [[ -z "${CLANG_FORMAT}" ]]; then
  CLANG_FORMAT="/usr/bin/clang-format"
else
  CLANG_FORMAT="${CLANG_FORMAT}"
fi

# remove any older patches from previous commits. Set to true or false.
# DELETE_OLD_PATCHES=false
DELETE_OLD_PATCHES=false

# only parse files with the extensions in FILE_EXTS. Set to true or false.
# if false every changed file in the commit will be parsed with clang-format.
# if true only files matching one of the extensions are parsed with clang-format.
# PARSE_EXTS=true
PARSE_EXTS=true

# file types to parse. Only effective when PARSE_EXTS is true.
# FILE_EXTS=".c .h .cpp .hpp"
FILE_EXTS=".c .h .cpp .hpp .cc .hh .cxx .m"

# exit on error
set -e

# check whether the given file matches any of the set extensions
matches_extension() {
    local filename=$(basename "$1")
    local extension=".${filename##*.}"
    local ext

    for ext in $FILE_EXTS; do [[ "$ext" == "$extension" ]] && return 0; done

    return 1
}

THIS_PATH="$(realpath "$0")"
THIS_DIR="$(dirname "$THIS_PATH")"

# Find all files in THIS_DIR which end in .c or .h as specified
# in the regular expression just below
FILE_LIST="$(find ./lib -name "*.h" -o -name "*.c" -o -name "*.cpp" | sed 's| |\\ |g')"

# Format each file.
# - NB: do NOT put quotes around `$FILE_LIST` below or else the `clang-format` command will
#   mistakenly see the entire blob of newline-separated file names as a SINGLE file name instead
#   of as a new-line separated list of *many* file names!
$CLANG_FORMAT --verbose -i --style=file $FILE_LIST

prefix="pre-commit-clang-format"
suffix="$(date +%s)"
patch="/tmp/$prefix-$suffix.patch"

# create one patch containing all changes to the files
git diff | while read line;
do
  echo "$line" >> "$patch"
done

# if no patch has been generated all is ok, clean up the file stub and exit
if [ ! -s "$patch" ]
then
    printf "Files in this merge request comply with the clang-format rules.\n"
    rm -f "$patch"
    exit 0
fi

# a patch has been created, notify the user and exit
printf "\nThe following differences were found between the code to merge "
printf "and the clang-format rules:\n\n"
cat "$patch"

exit 1
