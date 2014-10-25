#!/usr/bin/env bash
set -x
set -e

IS_COVR=$( python -c "print 'yes' if 'code-coverage' in '$JOB_NAME' else 'no'" )

if [[ $IS_COVR == "yes" ]]; then
  BASE="`pwd | sed -e 's|/|\\\/|g'`\\"
  (cd build && gcovr -x -f $BASE/core -f $BASE/daemon -f $BASE/rib -r ../ -o coverage.xml ./)
fi
