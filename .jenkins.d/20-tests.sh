#!/usr/bin/env bash
set -eo pipefail

# https://github.com/google/sanitizers/wiki/AddressSanitizerFlags
ASAN_OPTIONS="color=always"
ASAN_OPTIONS+=":check_initialization_order=1"
ASAN_OPTIONS+=":detect_stack_use_after_return=1"
ASAN_OPTIONS+=":strict_init_order=1"
ASAN_OPTIONS+=":strict_string_checks=1"
ASAN_OPTIONS+=":detect_invalid_pointer_pairs=2"
ASAN_OPTIONS+=":detect_container_overflow=0"
ASAN_OPTIONS+=":detect_leaks=0"
ASAN_OPTIONS+=":strip_path_prefix=${PWD}/"
export ASAN_OPTIONS

# https://www.boost.org/doc/libs/release/libs/test/doc/html/boost_test/runtime_config/summary.html
export BOOST_TEST_BUILD_INFO=1
export BOOST_TEST_COLOR_OUTPUT=1
export BOOST_TEST_DETECT_MEMORY_LEAK=0

ut_log_args() {
    echo --logger=HRF,test_suite,stdout:XML,all,build/xunit-log${1:+-$1}.xml
}

set -x

# Prepare environment
rm -rf ~/.ndn

# First run all tests as unprivileged user
./build/unit-tests-core $(ut_log_args core)
./build/unit-tests-daemon $(ut_log_args daemon)
./build/unit-tests-rib $(ut_log_args rib)
./build/unit-tests-tools $(ut_log_args tools)

# Then use sudo to run those tests that need superuser powers
sudo --preserve-env=ASAN_OPTIONS,BOOST_TEST_COLOR_OUTPUT \
    ./build/unit-tests-daemon -t TestPrivilegeHelper $(ut_log_args daemon-privilege-helper)
sudo --preserve-env=ASAN_OPTIONS,BOOST_TEST_COLOR_OUTPUT \
    ./build/unit-tests-daemon -t Face/*Ethernet* $(ut_log_args daemon-ethernet)
sudo --preserve-env=ASAN_OPTIONS,BOOST_TEST_COLOR_OUTPUT \
    ./build/unit-tests-daemon -t Face/TestUdpFactory $(ut_log_args daemon-udp)
