#!/bin/bash
#
# File: create-deb-package.sh
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

CMAKE_REALEASE=0;

while getopts "cs" opt; do
  case $opt in
    c)
      CMAKE_REALEASE=1;
      echo "Invalid option: -$OPTARG" >&2
     ;;
    \?)
      echo "Invalid option" >&2
      exit 1;
      ;;
  esac
done

if test $CMAKE_REALEASE -eq 1; then
  echo 'Creates the realease mode build script file with INSTALL_PREFIX="/usr"'
  sh  scripts/create-release-makefiles.sh
  echo "Generate debian package"
fi

for f in *.deb; do
  rm "$f"
done

cpack -G DEB --verbosy -DCPACK_PACKAGING_INSTALL_PREFIX="/usr" .

if test $? -eq 0; then
  rm -rf _CPack_Packages
fi
