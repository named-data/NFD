NFD: NDN Forwarding Daemon
==========================

## Prerequisites

* [ndn-cpp-dev library](https://github.com/named-data/ndn-cpp-dev) and
  its requirements:

    * `libcrypto`
    * `libsqlite3`
    * `libcrypto++`
    * `pkg-config`
    * Boost libraries (>= 1.48)
    * OSX Security framework (on OSX platform only)

    Refer to https://github.com/named-data/ndn-cpp-dev/blob/master/INSTALL.md
    for detailed installation instructions.

* `libpcap`

    Comes with base on Mac OS X 10.8 and 10.9:

    On Ubuntu >= 12.04:

        sudo apt-get install libpcap-dev

To build API documentation:

* `doxygen`

    On Mac OS X 10.8 and 10.9 with macports:

        sudo port install doxygen

    On Ubuntu >= 12.04:

        sudo apt-get install doxygen

## Build

The following commands should be used to build NFD:

    ./waf configure
    ./waf
    sudo ./waf install

Refer to `README.md` file for more options that can be used during `configure` stage and how to properly configure and run NFD.

In some configurations, configuration step may require small modification:

    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig
    ./waf configure
    ./waf
    sudo ./waf install
