#!@BASH@

VERSION="@VERSION@"

case "$1" in
  -h)
    echo Usage
    echo $0
    echo "  Stop NFD and RIB Management daemon"
    exit 0
    ;;
  -V)
    echo $VERSION
    exit 0
    ;;
  "") ;; # do nothing
  *)
    echo "Unrecognized option $1"
    exit 1
    ;;
esac

sudo killall nrd
sudo killall nfd
