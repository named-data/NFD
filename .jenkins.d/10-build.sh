#!/usr/bin/env bash
set -x
set -e

git submodule init
git submodule sync
git submodule update

# Cleanup
sudo ./waf distclean -j1 --color=yes

# Configure
COVERAGE=$( python -c "print '--with-coverage' if 'code-coverage' in '$JOB_NAME' else ''" )

CXXFLAGS="-std=c++03 -pedantic -Wall -Wno-long-long -O2 -g -Werror" \
  ./waf configure -j1 --color=yes --with-tests --without-pch $COVERAGE

# Build
./waf --color=yes -j1

# Install
sudo ./waf -j1 --color=yes install
