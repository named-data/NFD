#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

git submodule init
git submodule sync
git submodule update

if [[ $JOB_NAME == *"code-coverage" ]]; then
    COVERAGE="--with-coverage"
elif [[ -z $DISABLE_ASAN ]]; then
    ASAN="--with-sanitizer=address"
fi

# Cleanup
sudo env "PATH=$PATH" ./waf --color=yes distclean

if [[ $JOB_NAME != *"code-coverage" && $JOB_NAME != *"limited-build" ]]; then
  # Configure/build in optimized mode with tests
  ./waf --color=yes configure --with-tests
  ./waf --color=yes build -j${WAF_JOBS:-1}

  # Cleanup
  sudo env "PATH=$PATH" ./waf --color=yes distclean

  # Configure/build in optimized mode without tests, but with "other tests"
  ./waf --color=yes configure --with-other-tests
  ./waf --color=yes build -j${WAF_JOBS:-1}

  # Cleanup
  sudo env "PATH=$PATH" ./waf --color=yes distclean
fi

# Configure/build in debug mode with tests and without precompiled headers
./waf --color=yes configure --debug --with-tests --without-pch $ASAN $COVERAGE
./waf --color=yes build -j${WAF_JOBS:-1}

# (tests will be run against debug version)

# Install
sudo env "PATH=$PATH" ./waf --color=yes install
