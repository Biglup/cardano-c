#!/bin/bash
#
# File: create-debug-makefiles.sh
#
# Date: Sep 09, 2023
# Author: angel.castillo
#
# Copyright 2023 Biglup Labs
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

ARCH=""
CMAKELISTS_DIR="."
BUILD_DIR="."

for i in "$@"
do
  case $i in
    arch=*)
      ARCH="${i#*=}"
      shift
      ;;
    cmakelists=*)
      CMAKELISTS_DIR="${i#*=}"
      shift
      ;;
    *)
      CMAKE_USER_OPTIONS="${CMAKE_USER_OPTIONS} $i"
      shift
      ;;
  esac
done

CMAKE_USER_OPTIONS="${CMAKE_USER_OPTIONS} -DCMAKE_BUILD_TYPE=Debug -DTESTING_ENABLED=ON -DCMAKE_INSTALL_PREFIX=/usr"

if [ -n "${ARCH}" ]
then
  TOOLCHAIN_PATH="${CMAKELISTS_DIR}/cmake"
  CMAKE_OPTIONS=(-DBUILD_ARCHITECTURE=${ARCH} -DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_PATH}/CrossToolchain.cmake "${CMAKE_OPTIONS}")
fi

rm -rf ${BUILD_DIR}/CMakeCache.txt ${BUILD_DIR}/CMakeFiles
cmake "${CMAKE_OPTIONS[@]}" ${CMAKE_USER_OPTIONS} -B${BUILD_DIR} -H${CMAKELISTS_DIR}
