#!/bin/bash
#
# File: create-release-makefiles.sh
#
# Date: Sep 09, 2023
#
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

VERSION=$(<VERSION)

ARCH=""
CMAKELISTS_DIR="."
CMAKE_USER_OPTIONS=""

function get_version_fields_from_version {
  VERSION_FIELDS=($(echo ${VERSION} | awk '{print $1 " " $2 " " $3 }'))

  VERSION_MAJOR=${VERSION_FIELDS[0]}
  VERSION_MINOR=${VERSION_FIELDS[1]}
  VERSION_PATCH=${VERSION_FIELDS[2]}
}

function parse_version_file_content {

  VERSION=$(echo $VERSION | sed -E 's/VERSION_MAJOR=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/VERSION_MINOR=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/VERSION_PATCH=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/\n/''/g')
}

function create_release_notes {

  VERSION=$(echo $VERSION | sed -E 's/VERSION_MAJOR=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/VERSION_MINOR=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/VERSION_PATCH=/''/g')
  VERSION=$(echo $VERSION | sed -E 's/\n/''/g')
}

parse_version_file_content
get_version_fields_from_version ${VERSION}

TAG="v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"

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

parse_version_file_content
get_version_fields_from_version ${VERSION}

VERSION="-DVERSION_MAJOR=${VERSION_MAJOR} -DVERSION_MINOR=${VERSION_MINOR} -DVERSION_PATCH=${VERSION_PATCH}"

CMAKE_OPTIONS="${CMAKE_USER_OPTIONS} -DCMAKE_BUILD_TYPE=Release -DTESTING_ENABLED=OFF -DCMAKE_INSTALL_PREFIX=/usr -DDOXYGEN_ENABLED:BOOL=ON"

if [ -n "${ARCH}" ]
then
  CMAKE_OPTIONS="-DBUILD_ARCHITECTURE=${ARCH} -DCMAKE_TOOLCHAIN_FILE=cmake/CrossToolchain.cmake ${CMAKE_OPTIONS}"
fi

echo cmake ${CMAKE_OPTIONS} ${VERSION} ${CMAKELISTS_DIR}
cmake ${CMAKE_OPTIONS} ${VERSION} ${CMAKELISTS_DIR}
