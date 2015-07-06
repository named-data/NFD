#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

pushd /tmp >/dev/null

INSTALLED_VERSION=$((cd ndn-cxx && git rev-parse HEAD) 2>/dev/null || echo NONE)

sudo rm -Rf ndn-cxx-latest

git clone --depth 1 git://github.com/named-data/ndn-cxx ndn-cxx-latest

LATEST_VERSION=$((cd ndn-cxx-latest && git rev-parse HEAD) 2>/dev/null || echo UNKNOWN)

if [[ $INSTALLED_VERSION != $LATEST_VERSION ]]; then
    sudo rm -Rf ndn-cxx
    mv ndn-cxx-latest ndn-cxx
else
    sudo rm -Rf ndn-cxx-latest
fi

sudo rm -Rf /usr/local/include/ndn-cxx
sudo rm -f /usr/local/lib/libndn-cxx*
sudo rm -f /usr/local/lib/pkgconfig/libndn-cxx*

pushd ndn-cxx >/dev/null

./waf configure -j1 --color=yes --enable-shared --disable-static --without-osx-keychain
./waf -j1 --color=yes
sudo ./waf install -j1 --color=yes

popd >/dev/null
popd >/dev/null

if has Linux $NODE_LABELS; then
    sudo ldconfig
elif has FreeBSD $NODE_LABELS; then
    sudo ldconfig -a
fi
