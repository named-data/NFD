#!/usr/bin/env bash
set -x
set -e

git submodule init
git submodule sync
git submodule update

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in optimized mode with tests and precompiled headers
./waf -j1 --color=yes configure --with-tests
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in optimized mode without tests and with precompiled headers
./waf -j1 --color=yes configure
./waf -j1 --color=yes build

# Cleanup
sudo ./waf -j1 --color=yes distclean

# Configure/build in debug mode
if [[ "$JOB_NAME" == *"code-coverage" ]]; then
    COVERAGE="--with-coverage"
fi
./waf -j1 --color=yes configure --debug --with-tests --without-pch $COVERAGE
./waf -j1 --color=yes build

# (tests will be run against debug version)

# Install
sudo ./waf -j1 --color=yes install
