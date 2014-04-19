#!@BASH@

hasProcess() {
  local processName=$1

  if pgrep -x $processName >/dev/null
  then
    echo $processName
  fi
}

hasNFD=$(hasProcess nfd)
hasNRD=$(hasProcess nrd)

if [[ -n $hasNFD$hasNRD ]]
then
  echo 'NFD or NRD is already running...'
  exit 1
fi

sudo nfd &
sleep 2
nrd &
sleep 2
