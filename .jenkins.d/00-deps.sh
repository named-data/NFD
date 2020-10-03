#!/usr/bin/env bash
set -ex

if has OSX $NODE_LABELS; then
    FORMULAE=(boost openssl pkg-config)
    if has OSX-10.13 $NODE_LABELS || has OSX-10.14 $NODE_LABELS; then
        FORMULAE+=(python)
    fi

    if [[ -n $TRAVIS ]]; then
        # Travis images come with a large number of pre-installed
        # brew packages, don't waste time upgrading all of them
        brew list --versions "${FORMULAE[@]}" || brew update
        for FORMULA in "${FORMULAE[@]}"; do
            brew list --versions "$FORMULA" || brew install "$FORMULA"
        done
        # Ensure /usr/local/opt/openssl exists
        brew reinstall openssl
    else
        brew update
        brew upgrade
        brew install "${FORMULAE[@]}"
        brew cleanup
    fi

elif has Ubuntu $NODE_LABELS; then
    sudo apt-get -qq update
    sudo apt-get -qy install build-essential pkg-config python3-minimal \
                             libboost-all-dev libssl-dev libsqlite3-dev \
                             libpcap-dev libsystemd-dev

    if [[ $JOB_NAME == *"code-coverage" ]]; then
        sudo apt-get -qy install gcovr lcov
    fi

elif has CentOS-8 $NODE_LABELS; then
    sudo dnf config-manager --enable PowerTools
    sudo dnf -y install gcc-c++ libasan pkgconf-pkg-config python3 \
                        boost-devel openssl-devel sqlite-devel \
                        libpcap-devel systemd-devel
fi
