#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

# Prepare environment
rm -Rf ~/.ndnx ~/.ndn

if has OSX $NODE_LABELS; then
    security unlock-keychain -p named-data
fi

ndnsec-keygen "/tmp/jenkins/$NODE_NAME" | ndnsec-install-cert -

count=0

# Helper function
run_tests() {
    local sudo=
    if [[ $1 == sudo ]]; then
        sudo=$1
        shift
    fi

    local module=$1
    shift

    if [[ -n $XUNIT ]]; then
        ${sudo} ./build/unit-tests-${module} -l all "$@" -- --log_format2=XML --log_sink2="build/xunit-${count}-${module}${sudo:+-}${sudo}.xml"
        ((count+=1))
    else
        ${sudo} ./build/unit-tests-${module} -l test_suite "$@"
    fi
}

# First run all tests as unprivileged user
run_tests core
run_tests daemon
run_tests rib
run_tests tools

# Then use sudo to run those tests that need superuser powers
run_tests sudo core -t TestPrivilegeHelper
run_tests sudo daemon -t Face/TestEthernetFactory,TestEthernetTransport
run_tests sudo daemon -t Mgmt/TestGeneralConfigSection/UserAndGroupConfig,NoUserConfig
run_tests sudo daemon -t Mgmt/TestFaceManager/ProcessConfig/ProcessSectionUdp,ProcessSectionUdpMulticastReinit,ProcessSectionEther,ProcessSectionEtherMulticastReinit

unset count
