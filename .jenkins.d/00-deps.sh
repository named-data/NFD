#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

if has OSX $NODE_LABELS; then
    FORMULAE=(boost openssl pkg-config)
    brew update
    if [[ -n $TRAVIS ]]; then
        # travis images come with a large number of brew packages
        # pre-installed, don't waste time upgrading all of them
        for FORMULA in "${FORMULAE[@]}"; do
            brew outdated $FORMULA || brew upgrade $FORMULA
        done
    else
        brew upgrade
    fi
    brew install "${FORMULAE[@]}"
    brew cleanup
fi

if has Ubuntu $NODE_LABELS; then
    sudo apt-get -qq update
    sudo apt-get -qy install build-essential pkg-config libboost-all-dev \
                             libsqlite3-dev libssl-dev libpcap-dev

    if [[ $JOB_NAME == *"code-coverage" ]]; then
        sudo apt-get -qy install lcov libgd-perl python-setuptools
        sudo easy_install --upgrade gcovr
    fi
fi

if has CentOS $NODE_LABELS; then
    sudo yum -y install yum-utils pkgconfig libpcap-devel \
                        openssl-devel libtranslit-icu \
                        python-devel sqlite-devel \
                        devtoolset-7-libasan-devel \
                        devtoolset-7-liblsan-devel
    sudo yum -y groupinstall 'Development Tools'

    svn checkout https://github.com/cmscaltech/sandie-ndn/trunk/packaging/RPMS/x86_64
    sudo rpm -U --force x86_64/boost-license*
    sudo rpm -U --force x86_64/boost-doc-html*
    sudo rpm -U --force x86_64/libboost*
    sudo rpm -U --force x86_64/boost-devel*
fi
