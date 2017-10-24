#!/usr/bin/env bash
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

set -x

# Prepare environment
rm -Rf ~/.ndn

if has OSX $NODE_LABELS; then
    security unlock-keychain -p named-data
fi

ndnsec-keygen "/tmp/jenkins/$NODE_NAME" | ndnsec-install-cert -

BOOST_VERSION=$(python -c "import sys; sys.path.append('build/c4che'); import _cache; print(_cache.BOOST_VERSION_NUMBER);")

ut_log_args() {
    if (( BOOST_VERSION >= 106200 )); then
        echo --logger=HRF,test_suite,stdout:XML,all,build/xunit-${1:-report}.xml
    else
        if [[ -n $XUNIT ]]; then
            echo --log_level=all $( (( BOOST_VERSION >= 106000 )) && echo -- ) \
                 --log_format2=XML --log_sink2=build/xunit-${1:-report}.xml
        else
            echo --log_level=test_suite
        fi
    fi
}

ASAN_OPTIONS="color=always"
ASAN_OPTIONS+=":detect_leaks=false"
ASAN_OPTIONS+=":detect_stack_use_after_return=true"
ASAN_OPTIONS+=":check_initialization_order=true"
ASAN_OPTIONS+=":strict_init_order=true"
ASAN_OPTIONS+=":detect_invalid_pointer_pairs=1"
ASAN_OPTIONS+=":detect_container_overflow=false"
ASAN_OPTIONS+=":strict_string_checks=true"
ASAN_OPTIONS+=":strip_path_prefix=${PWD}/"
export ASAN_OPTIONS

# First run all tests as unprivileged user
./build/unit-tests-core $(ut_log_args core)
./build/unit-tests-daemon $(ut_log_args daemon)
./build/unit-tests-rib $(ut_log_args rib)
./build/unit-tests-tools $(ut_log_args tools)

# Then use sudo to run those tests that need superuser powers
sudo -E ./build/unit-tests-core -t TestPrivilegeHelper $(ut_log_args core-privilege)
sudo -E ./build/unit-tests-daemon -t Face/*Ethernet* $(ut_log_args daemon-ethernet)
sudo -E ./build/unit-tests-daemon -t Face/TestUdpFactory $(ut_log_args daemon-udp-factory)
sudo -E ./build/unit-tests-daemon -t Mgmt/TestGeneralConfigSection/UserAndGroupConfig,NoUserConfig $(ut_log_args daemon-user-config)
