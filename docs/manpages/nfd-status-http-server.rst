nfd-status-http-server
======================

Usage
-----

::

    nfd-status-http-server [-h] [-p <PORT>] [-a <IPADDR>] [-r] [-v]

Description
-----------

``nfd-status-http-server`` is a daemon that enables retrieval of NFD status via HTTP protocol.

Options
-------

``-h``
  Show this help message and exit.

``-p <PORT>``
  HTTP server port number (default is 8080).

``-a <IPADDR>``
  HTTP server IP address (default is 127.0.0.1).

``-r``
  Enable HTTP robots to crawl (disabled by default).

``-v``
  Verbose mode.

Examples
--------

Enable NFD HTTP status server on all IPv4 interfaces::

    nfd-status-http-server -p 80 -a 0.0.0.0

Enable NFD HTTP status server on all IPv6 interfaces::

    nfd-status-http-server -p 80 -a ::
