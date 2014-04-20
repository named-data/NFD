nfd
===

Usage
-----

::

    nfd [options]


Description
-----------

NFD forwarding daemon.


Options:
--------

``--help``
  Print this help message.

``--modules``
  List available logging modules

``--config <path/to/nfd.conf>``
  Specify the path to nfd configuration file (default: ``${SYSCONFDIR}/ndn/nfd.conf``).

Examples
--------

Start NFD forwarding daemon as super user and use a configuration file from the current
working directory.

::

    sudo nfd --config nfd.conf
