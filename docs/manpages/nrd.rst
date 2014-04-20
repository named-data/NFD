NRD (NFD RIB Daemon)
====================

Usage
-----

::

    nrd [options]


Description
-----------

NRD is a complementary daemon designed to work in parallel with NFD and to provide
services for Routing Information Base management and constructing NFD's FIB:

* prefix registration from applications;
* manual prefix registration;
* prefix registrations from multiple dynamic routing protocols.

NRD's runtime settings may be modified via `rib_*` sections in NFD's configuration file.

Options:
--------

``--help``
  Print this help message.

``--modules``
  List available logging modules

``--config <path/to/nfd.conf>``
  Specify the path to nfd configuration file (default: ``${SYSCONFDIR}/ndn/nfd.conf``).
