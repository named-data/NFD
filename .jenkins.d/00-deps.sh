#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

if has OSX $NODE_LABELS; then
    if has OSX-10.8 $NODE_LABELS; then
        EXTRA_FLAGS=--c++11
    fi

    set -x
    brew update
    brew upgrade
    brew install boost pkg-config cryptopp $EXTRA_FLAGS
    brew cleanup
fi

if has Ubuntu $NODE_LABELS; then
    set -x
    sudo apt-get -qq update
    sudo apt-get -qq install build-essential pkg-config libboost-all-dev \
                             libcrypto++-dev libsqlite3-dev libpcap-dev
fi
