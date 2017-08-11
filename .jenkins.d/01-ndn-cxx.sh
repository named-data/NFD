#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

pushd "${CACHE_DIR:-/tmp}" >/dev/null

INSTALLED_VERSION=
if has OSX $NODE_LABELS; then
    BOOST=$(brew ls --versions boost)
    OLD_BOOST=$(cat boost.txt || :)
    if [[ $OLD_BOOST != $BOOST ]]; then
        echo "$BOOST" > boost.txt
        INSTALLED_VERSION=NONE
    fi
fi

if [[ -z $INSTALLED_VERSION ]]; then
    INSTALLED_VERSION=$(git -C ndn-cxx rev-parse HEAD 2>/dev/null || echo NONE)
fi

sudo rm -Rf ndn-cxx-latest

git clone --depth 1 git://github.com/named-data/ndn-cxx ndn-cxx-latest

LATEST_VERSION=$(git -C ndn-cxx-latest rev-parse HEAD 2>/dev/null || echo UNKNOWN)

if [[ $INSTALLED_VERSION != $LATEST_VERSION ]]; then
    sudo rm -Rf ndn-cxx
    mv ndn-cxx-latest ndn-cxx
else
    sudo rm -Rf ndn-cxx-latest
fi

sudo rm -f /usr/local/bin/ndnsec*
sudo rm -fr /usr/local/include/ndn-cxx
sudo rm -f /usr/local/lib/libndn-cxx*
sudo rm -f /usr/local/lib/pkgconfig/libndn-cxx.pc

pushd ndn-cxx >/dev/null

./waf configure --color=yes --enable-shared --disable-static --without-osx-keychain
./waf build --color=yes -j${WAF_JOBS:-1}
sudo env "PATH=$PATH" ./waf install --color=yes

popd >/dev/null
popd >/dev/null

if has Linux $NODE_LABELS; then
    sudo ldconfig
elif has FreeBSD10 $NODE_LABELS; then
    sudo ldconfig -m
fi
