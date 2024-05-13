nfd-status-http-server
======================

Synopsis
--------

**nfd-status-http-server** [**-h**] [**-a** *IPADDR*] [**-p** *PORT*] [**-r**] [**-v**]

Description
-----------

``nfd-status-http-server`` is a daemon that enables the retrieval of NFD's status over HTTP.

Options
-------

``-h``
  Show help message and exit.

``-a <IPADDR>``
  HTTP server IP address (default is 127.0.0.1).

``-p <PORT>``
  HTTP server port number (default is 6380).

``-r``
  Enable HTTP robots to crawl (disabled by default).

``-v``
  Verbose mode.

Examples
--------

Start NFD's HTTP status server on all IPv4 interfaces, port 80 (requires root)::

    nfd-status-http-server -a 0.0.0.0 -p 80

Start NFD's HTTP status server on all IPv6 interfaces, port 8000::

    nfd-status-http-server -a :: -p 8000
