#!/usr/bin/env bash
set -eo pipefail

APT_PKGS=(
    dpkg-dev
    g++
    libboost-chrono-dev
    libboost-date-time-dev
    libboost-dev
    libboost-log-dev
    libboost-program-options-dev
    libboost-stacktrace-dev
    libboost-test-dev
    libboost-thread-dev
    libpcap-dev
    libsqlite3-dev
    libssl-dev
    libsystemd-dev
    pkgconf
    python3
)
DNF_PKGS=(
    boost-devel
    gcc-c++
    libasan
    libpcap-devel
    lld
    openssl-devel
    pkgconf
    python3
    sqlite-devel
    systemd-devel
)
FORMULAE=(boost openssl pkgconf)
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
    export HOMEBREW_NO_ENV_HINTS=1
    if [[ -n $GITHUB_ACTIONS ]]; then
        export HOMEBREW_NO_INSTALLED_DEPENDENTS_CHECK=1
    fi
    brew update
    brew install --formula "${FORMULAE[@]}"
elif [[ $ID_LIKE == *debian* ]]; then
    sudo apt-get update -qq
    sudo apt-get install -qy --no-install-recommends "${APT_PKGS[@]}"
elif [[ $ID_LIKE == *fedora* ]]; then
    sudo dnf install -y "${DNF_PKGS[@]}"
fi

if (( ${#PIP_PKGS[@]} )); then
    pip3 install --user --upgrade --upgrade-strategy=eager "${PIP_PKGS[@]}"
fi
