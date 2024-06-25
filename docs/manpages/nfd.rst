nfd
===

Synopsis
--------

| **nfd** [**-c**\|\ **\--config** *file*]
| **nfd** **-h**\|\ **\--help**
| **nfd** **-m**\|\ **\--modules**
| **nfd** **-V**\|\ **\--version**

Description
-----------

NFD is a network forwarder that implements the Named Data Networking (NDN) protocol.

Options
-------

.. option:: -c <file>, --config <file>

    Specify the path to NFD's configuration file.

.. option:: -h, --help

    Print help message and exit.

.. option:: -m, --modules

    List available logging modules.

.. option:: -V, --version

    Show version information and exit.

Exit Status
-----------

0
    No error.

1
    An unspecified error occurred.

2
    Malformed command line, e.g., invalid, missing, or unknown argument.

4
    Insufficient privileges.

Examples
--------

``sudo nfd --config nfd.conf``
    Start NFD as super user with a configuration file from the current directory.

See Also
--------

:manpage:`nfdc(1)`
