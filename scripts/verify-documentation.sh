#!/bin/bash

# Run Sphinx to build the documentation
./scripts/create-docs-makefiles.sh > sphinx-build.log 2>&1

# Check for warnings in the Sphinx output
if grep -i ": warning:" sphinx-build.log; then
    echo "Sphinx documentation generation has warnings."
    exit 1
else
    echo "Sphinx documentation generated without warnings."
fi