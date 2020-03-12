#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

git submodule sync
git submodule update --init

if [[ $JOB_NAME == *"code-coverage" ]]; then
    COVERAGE="--with-coverage"
elif [[ -z $DISABLE_ASAN ]]; then
    ASAN="--with-sanitizer=address"
fi

# Cleanup
sudo_preserve_env PATH -- ./waf --color=yes distclean

if [[ $JOB_NAME != *"code-coverage" && $JOB_NAME != *"limited-build" ]]; then
    # Build in release mode with tests and without precompiled headers
    ./waf --color=yes configure --with-tests --without-pch
    ./waf --color=yes build -j$WAF_JOBS

    # Cleanup
    sudo_preserve_env PATH -- ./waf --color=yes distclean

    # Build in release mode without tests, but with "other tests"
    ./waf --color=yes configure --with-other-tests
    ./waf --color=yes build -j$WAF_JOBS

    # Cleanup
    sudo_preserve_env PATH -- ./waf --color=yes distclean
fi

# Build in debug mode with tests
./waf --color=yes configure --debug --with-tests $ASAN $COVERAGE
./waf --color=yes build -j$WAF_JOBS

# (tests will be run against the debug version)

# Install
sudo_preserve_env PATH -- ./waf --color=yes install
