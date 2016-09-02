#!/usr/bin/env bash
set -x
set -e

JDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "$JDIR"/util.sh

if [[ $JOB_NAME == *"code-coverage" ]]; then
    gcovr --output=coverage.xml \
          --filter="$PWD/core" --filter="$PWD/daemon" --filter="$PWD/rib" --filter="$PWD/tools" \
          --root=. \
          --xml \
          build
fi
