#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

if [[ "$JOB_NAME" == *"code-coverage" ]]; then
  BASE="`pwd | sed -e 's|/|\\\/|g'`\\"
  (cd build && gcovr -x -f $BASE/core -f $BASE/daemon -f $BASE/rib -r ../ -o coverage.xml ./)
fi
