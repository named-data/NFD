#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

# Prepare environment
rm -Rf ~/.ndnx ~/.ndn

if has OSX $NODE_LABELS; then
  security unlock-keychain -p "named-data"
  sudo chgrp admin /dev/bpf*
  sudo chmod g+rw /dev/bpf*
fi

if has Linux $NODE_LABELS; then
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-core || true
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-daemon || true
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-rib || true
fi

ndnsec-keygen "/tmp/jenkins/$NODE_NAME" | ndnsec-install-cert -

# Run unit tests
# Core
if [[ -n $XUNIT ]]; then
  ./build/unit-tests-core -l all -- --log_format2=XML --log_sink2=build/xunit-core-report.xml
  sudo ./build/unit-tests-core -t TestPrivilegeHelper -l all -- --log_format2=XML --log_sink2=build/xunit-core-sudo-report.xml

  ./build/unit-tests-daemon -l all -- --log_format2=XML --log_sink2=build/xunit-daemon-report.xml

  ./build/unit-tests-rib -l all -- --log_format2=XML --log_sink2=build/xunit-rib-report.xml
else
  ./build/unit-tests-core -l test_suite
  sudo ./build/unit-tests-core -t TestPrivilegeHelper -l test_suite

  ./build/unit-tests-daemon -l test_suite

  ./build/unit-tests-rib -l test_suite
fi
