#!/usr/bin/env bash
set -eo pipefail

APT_PKGS=(build-essential pkg-config python3-minimal
          libboost-all-dev libssl-dev libsqlite3-dev
          libpcap-dev libsystemd-dev)
FORMULAE=(boost openssl pkg-config)
PIP_PKGS=()
case $JOB_NAME in
    *code-coverage)
        APT_PKGS+=(lcov python3-pip)
        PIP_PKGS+=('gcovr~=5.2')
        ;;
    *Docs)
        APT_PKGS+=(doxygen graphviz python3-pip)
        FORMULAE+=(doxygen graphviz)
        PIP_PKGS+=(sphinx sphinxcontrib-doxylink)
        ;;
esac

set -x

if [[ $ID == macos ]]; then
    if [[ -n $GITHUB_ACTIONS ]]; then
        export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
    fi
    brew update
    brew install --formula "${FORMULAE[@]}"
elif [[ $ID_LIKE == *debian* ]]; then
    sudo apt-get -qq update
    sudo apt-get -qy install "${APT_PKGS[@]}"
elif [[ $ID_LIKE == *fedora* ]]; then
    sudo dnf -y install gcc-c++ libasan lld pkgconf-pkg-config python3 \
                        boost-devel openssl-devel sqlite-devel \
                        libpcap-devel systemd-devel
fi

if (( ${#PIP_PKGS[@]} )); then
    pip3 install --user --upgrade --upgrade-strategy=eager "${PIP_PKGS[@]}"
fi
