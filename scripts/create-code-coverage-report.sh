#!/bin/bash
#
# File: create-code-coverage-report.sh
#
# Date: Sep 09, 2024
#
# Author: luisd.bianchi
#
# Copyright 2024 Biglup Labs
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

# Function to check if a command exists
command_exists () {
    type "$1" &> /dev/null ;
}

# Check if lcov is installed
if command_exists lcov; then
    echo "lcov found, proceeding with code coverage generation."

    # Your existing script to generate code coverage report
    rm -rf ./build/debug/coverage/
    mkdir -p ./build/debug/coverage/
    lcov --directory ./build/debug/lib/ --capture --output-file ./build/debug/coverage/code_coverage.info -rc lcov_branch_coverage=1
    genhtml ./build/debug/coverage/code_coverage.info --branch-coverage --output-directory ./build/debug/coverage/code_coverage_report/
    if command_exists open; then
        open ./build/debug/coverage/code_coverage_report/index.html
    elif command_exists xdg-open; then
        xdg-open ./build/debug/coverage/code_coverage_report/index.html
    else
        echo "Coverage report generated at ./build/debug/coverage/code_coverage_report/index.html"
        echo "Please open it in your web browser."
    fi
else
    # lcov not found, instruct the user to install it
    echo "lcov not found. Please install lcov to generate code coverage reports."
    echo "On Debian/Ubuntu-based systems, you can install lcov using:"
    echo "sudo apt-get install lcov"
fi