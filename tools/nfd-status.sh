#!@BASH@
if [[ $# -gt 0 ]]; then
  echo 'nfd-status command line options are deprecated.' >&2
  echo 'Use the `nfdc` subcommands instead. See `nfdc help` for details.' >&2
  exec "$(dirname "$0")/nfdc" legacy-nfd-status "$@"
else
  # nfd-status is an alias of `nfdc status report`
  exec "$(dirname "$0")/nfdc" status report
fi
