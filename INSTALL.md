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

    Comes with base on OS X 10.8 and 10.9:

    On Ubuntu >= 12.04:

        sudo apt-get install libpcap-dev

To build manpages and API documentation:

* `doxygen`
* `graphviz`
* `python-sphinx`

    On OS X 10.8 and 10.9 with macports:

        sudo port install doxygen graphviz py27-sphinx sphinx_select
        sudo port select sphinx py27-sphinx

    On Ubuntu >= 12.04:

        sudo apt-get install doxygen graphviz python-sphinx


## Build

The following commands should be used to build NFD:

    ./waf configure
    ./waf
    sudo ./waf install

Refer to `README.md` file for more options that can be used during `configure` stage and
how to properly configure and run NFD.

In some configurations, configuration step may require small modification.  For example,
on OSX that uses macports (correct the path if macports was not installed in the default
path `/opt/local`):

    export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

On some Linux distributions (e.g., Fedora 20):

    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

# Building API documentation

The following commands can be used to build API documentation in `build/docs/doxygen`

    ./waf doxygen

Note that manpages are automatically created and installed during the normal build process
(e.g., during `./waf` and `./waf install`), if `python-sphinx` module is detected during
`./waf configure` stage.  By default, manpages are installed into `${PREFIX}/share/man`
(where default value for `PREFIX` is `/usr/local`).  This location can be changed during
`./waf configure` stage using `--prefix`, `--datarootdir`, or `--mandir` options.

For more details, refer to `./waf --help`.

Additional documentation in `build/docs` can be built using (requires `python-sphinx` package)

    ./waf sphinx
