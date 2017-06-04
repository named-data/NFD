#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

if [[ $JOB_NAME == *"code-coverage" ]]; then
    gcovr --object-directory=build \
          --output=build/coverage.xml \
          --exclude="$PWD/(tests|websocketpp)" \
          --root=. \
          --xml

    # Generate also a detailed HTML output, but using lcov (better results)
    lcov --quiet \
         --capture \
         --directory . \
         --no-external \
         --rc lcov_branch_coverage=1 \
         --output-file build/coverage-with-tests-and-websocketpp.info

    lcov --quiet \
         --remove build/coverage-with-tests-and-websocketpp.info "$PWD/websocketpp/*" \
         --rc lcov_branch_coverage=1 \
         --output-file build/coverage-with-tests.info

    lcov --quiet \
         --remove build/coverage-with-tests.info "$PWD/tests/*" \
         --rc lcov_branch_coverage=1 \
         --output-file build/coverage.info

    genhtml --branch-coverage \
            --demangle-cpp \
            --frames \
            --legend \
            --output-directory build/coverage \
            --title "NFD unit tests" \
            build/coverage.info
fi
