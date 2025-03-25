#!/usr/bin/env bash
set -eo pipefail

if [[ -z $DISABLE_ASAN ]]; then
    ASAN="--with-sanitizer=address"
fi
if [[ $JOB_NAME == *code-coverage ]]; then
    COVERAGE="--with-coverage"
fi

set -x

if [[ $JOB_NAME != *code-coverage && $JOB_NAME != *limited-build ]]; then
    # Build in release mode with tests and without precompiled headers
    ./waf --color=yes configure --with-tests --without-pch
    ./waf --color=yes build

    # Cleanup
    ./waf --color=yes distclean

    # Build in release mode with "other tests" only
    ./waf --color=yes configure --with-other-tests
    ./waf --color=yes build

    # Cleanup
    ./waf --color=yes distclean
fi

# Build in debug mode with tests
./waf --color=yes configure --debug --with-tests $ASAN $COVERAGE
./waf --color=yes build

# Install
sudo ./waf --color=yes install
