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
case $JOB_NAME in
    *code-coverage)
        APT_PKGS+=(lcov libjson-xs-perl)
        ;;
    *Docs)
        APT_PKGS+=(doxygen graphviz)
        FORMULAE+=(doxygen graphviz)
        ;;
esac

install_uv() {
    if [[ -z $GITHUB_ACTIONS && $ID_LIKE == *debian* ]]; then
        sudo apt-get install -qy --no-install-recommends pipx
        pipx upgrade uv || pipx install uv
    fi
}

set -x

if [[ $ID == macos ]]; then
    export HOMEBREW_COLOR=1
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

case $JOB_NAME in
    *code-coverage)
        install_uv
        ;;
    *Docs)
        install_uv
        export FORCE_COLOR=1
        export UV_NO_MANAGED_PYTHON=1
        uv tool install sphinx --upgrade --with-requirements docs/requirements.txt
        ;;
esac
