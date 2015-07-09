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
    BOOST_PKG=libboost-all-dev
    if has Ubuntu-12.04 $NODE_LABELS; then
        BOOST_PKG=libboost1.48-all-dev
    fi

    set -x
    sudo apt-get update -qq -y
    sudo apt-get -qq -y install build-essential pkg-config $BOOST_PKG \
                                libcrypto++-dev libsqlite3-dev libpcap-dev
fi
