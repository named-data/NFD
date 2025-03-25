#!/usr/bin/env bash
set -eo pipefail

[[ $JOB_NAME == *code-coverage ]] || exit 0

export FORCE_COLOR=1
export UV_NO_MANAGED_PYTHON=1

set -x

# Generate an XML report (Cobertura format) and a detailed HTML report using gcovr
# Note: trailing slashes are important in the paths below. Do not remove them!
uvx gcovr@5.2 \
    --object-directory build \
    --exclude tests/ \
    --exclude websocketpp/ \
    --exclude-throw-branches \
    --exclude-unreachable-branches \
    --cobertura build/coverage.xml \
    --html-details build/gcovr/ \
    --print-summary

# Generate a detailed HTML report using lcov
lcov --quiet \
     --capture \
     --directory . \
     --exclude "$PWD/tests/*" \
     --exclude "$PWD/websocketpp/*" \
     --no-external \
     --rc lcov_branch_coverage=1 \
     --output-file build/coverage.info

genhtml --branch-coverage \
        --demangle-cpp \
        --legend \
        --output-directory build/lcov \
        --title "NFD unit tests" \
        build/coverage.info
