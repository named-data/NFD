nfd-status
==========

Usage
-----

::

    nfd-status

Description
-----------

``nfd-status`` is an alias of ``nfdc status report``, which generates a comprehensive report of NFD status.

Deprecated Options
------------------

nfd-status used to support the following command line options.
They have been deprecated and will be removed in a future release of NFD.

``-v``
  Print general status. Use ``nfdc status show`` instead.

``-c``
  Print channel list. Use ``nfdc channel list`` instead.

``-f``
  Print face list. Use ``nfdc face list`` instead.

``-b``
  Print FIB entries. Use ``nfdc fib list`` instead.

``-r``
  Print RIB entries. Use ``nfdc route list`` instead.

``-s``
  Print strategy choices. Use ``nfdc strategy list`` instead.

``-x``
  Generate a comprehensive report of NFD status in XML format. Use ``nfdc status report xml`` instead.

``-h``
  Print usage information.

``-V``
  Show version information of nfd-status and exit.

Exit Codes
----------

0: Success

1: An unspecified error occurred

2: Malformed command line
