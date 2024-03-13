#!/usr/bin/env bash
set -exo pipefail

pushd "$CACHE_DIR" >/dev/null

INSTALLED_VERSION=
if [[ $ID == macos ]]; then
    BOOST=$(brew list --formula --versions boost)
    OLD_BOOST=$(cat boost.txt || :)
    if [[ $OLD_BOOST != $BOOST ]]; then
        echo "$BOOST" > boost.txt
        INSTALLED_VERSION=NONE
    fi
fi

if [[ -z $INSTALLED_VERSION ]]; then
    INSTALLED_VERSION=$(git -C ndn-cxx rev-parse HEAD 2>/dev/null || echo NONE)
fi

sudo rm -rf ndn-cxx-latest
git clone --depth 1 https://github.com/named-data/ndn-cxx.git ndn-cxx-latest
LATEST_VERSION=$(git -C ndn-cxx-latest rev-parse HEAD 2>/dev/null || echo UNKNOWN)

if [[ $INSTALLED_VERSION != $LATEST_VERSION ]]; then
    sudo rm -rf ndn-cxx
    mv ndn-cxx-latest ndn-cxx
else
    sudo rm -rf ndn-cxx-latest
fi

sudo rm -f /usr/local/bin/ndnsec*
sudo rm -fr /usr/local/include/ndn-cxx
sudo rm -f /usr/local/lib{,64}/libndn-cxx*
sudo rm -f /usr/local/lib{,64}/pkgconfig/libndn-cxx.pc

pushd ndn-cxx >/dev/null

./waf --color=yes configure
./waf --color=yes build
sudo ./waf --color=yes install

popd >/dev/null
popd >/dev/null

if [[ $ID_LIKE == *linux* ]]; then
    if [[ $(uname -m) == x86_64 && -d /usr/lib64 ]]; then
        sudo tee /etc/ld.so.conf.d/ndn.conf >/dev/null <<< /usr/local/lib64
    fi
    sudo ldconfig
fi
