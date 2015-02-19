#!/usr/bin/env bash
set -x
set -e

# Prepare environment
rm -Rf ~/.ndnx ~/.ndn

echo $NODE_LABELS
IS_OSX=$( python -c "print 'yes' if 'OSX' in '$NODE_LABELS'.strip().split(' ') else 'no'" )
IS_LINUX=$( python -c "print 'yes' if 'Linux' in '$NODE_LABELS'.strip().split(' ') else 'no'" )

if [[ $IS_OSX == "yes" ]]; then
  security unlock-keychain -p "named-data"
  sudo chgrp admin /dev/bpf*
  sudo chmod g+rw /dev/bpf*
fi
if [[ $IS_LINUX = "yes" ]]; then
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-core || true
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-daemon || true
  sudo setcap cap_net_raw,cap_net_admin=eip `pwd`/build/unit-tests-rib || true
fi

ndnsec-keygen "/tmp/jenkins/$NODE_NAME" | ndnsec-install-cert -

# Run unit tests
# Core
./build/unit-tests-core -l test_suite
sudo ./build/unit-tests-core -t TestPrivilegeHelper -l test_suite

# Daemon
./build/unit-tests-daemon -l test_suite

# RIB
./build/unit-tests-rib -l test_suite
