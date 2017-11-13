nfd
===

Usage
-----

::

    nfd [options]

Description
-----------

NFD forwarding daemon.

Options
-------

``-c <path/to/nfd.conf>`` or ``--config <path/to/nfd.conf>``
  Specify the path to nfd configuration file (default: ``${SYSCONFDIR}/ndn/nfd.conf``).

``-m`` or ``--modules``
  List available logging modules.

``-h`` or ``--help``
  Print help message and exit.

``-V`` or ``--version``
  Show version information and exit.

Exit status
-----------

0: No error.

1: An unspecified error occurred.

2: Malformed command line, e.g., invalid, missing, or unknown argument.

4: Insufficient privileges.

Examples
--------

Start NFD forwarding daemon as super user and use a configuration file from the current
working directory.

::

    sudo nfd --config nfd.conf
