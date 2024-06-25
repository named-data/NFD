nfd-status-http-server
======================

Synopsis
--------

| **nfd-status-http-server** [**-a**\|\ **\--address** *address*] [**-p**\|\ **\--port** *port*] \
                             [**-f**\|\ **\--workdir** *dir*] [**-r**\|\ **\--robots**] [**-v**\|\ **\--verbose**]
| **nfd-status-http-server** **-h**\|\ **\--help**
| **nfd-status-http-server** **-V**\|\ **\--version**

Description
-----------

:program:`nfd-status-http-server` is a daemon that enables the retrieval of NFD's status over HTTP.

Options
-------

.. option:: -a <address>, --address <address>

    HTTP server IP address (default is 127.0.0.1).

.. option:: -p <port>, --port <port>

    HTTP server port number (default is 6380).

.. option:: -f <dir>, --workdir <dir>

    Set the server's working directory.

.. option:: -r, --robots

    Enable HTTP robots to crawl (disabled by default).

.. option:: -v, --verbose

    Enable verbose mode.

.. option:: -h, --help

    Print help message and exit.

.. option:: -V, --version

    Show version information and exit.

Examples
--------

``nfd-status-http-server -a 0.0.0.0 -p 80``
    Start NFD's HTTP status server on all IPv4 interfaces, port 80 (requires privileges).

``nfd-status-http-server -a :: -p 8000``
    Start NFD's HTTP status server on all IPv6 interfaces, port 8000.
