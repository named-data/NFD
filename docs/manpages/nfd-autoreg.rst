nfd-autoreg
===========

Synopsis
--------

| **nfd-autoreg** [**-i**\|\ **\--prefix** *prefix*]... [**-a**\|\ **\--all-faces-prefix** *prefix*]...
|                 [**-b**\|\ **\--blacklist** *network*]... [**-w**\|\ **\--whitelist** *network*]... \
                  [**-c**\|\ **\--cost** *cost*]
| **nfd-autoreg** **-h**\|\ **\--help**
| **nfd-autoreg** **-V**\|\ **\--version**

Description
-----------

:program:`nfd-autoreg` is a daemon application that automatically registers the specified
prefix(es) when new on-demand faces are created (i.e., when a new UDP face is created as
the result of an incoming packet or when a new TCP face is created as the result of an
incoming connection).

Options
-------

.. option:: -i <prefix>, --prefix <prefix>

    Prefix that should be automatically registered when a new remote face is created.
    Can be repeated multiple times to specify additional prefixes.

.. option:: -b <network>, --blacklist <network>

    Blacklisted network, e.g., 192.168.2.32/30 or ::1/128. Can be repeated multiple times
    to specify multiple blacklisted networks.

    The prefixes will be auto-registered only when the remote IP address is NOT inside the
    specified range(s), but is inside the range defined by the whitelist(s), if any.

    Default: none.

.. option:: -w <network>, --whitelist <network>

    Whitelisted network, e.g., 192.168.2.0/24 or ::1/128. Can be repeated multiple times
    to specify multiple whitelisted networks.

    The prefixes will be auto-registered only when the remote IP address is inside the
    specified range(s), excluding any blacklisted range(s).

    Default: 0.0.0.0/0 and ::/0.

.. option:: -c <cost>, --cost <cost>

    RIB cost to assign to auto-registered prefixes. If not specified, the cost is set to 255.

.. option:: -h, --help

    Print help message and exit.

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

``nfd-autoreg -i /app1/video -i /app2/pictures -b 10.0.0.0/8``
    Auto-register two prefixes for any newly created on-demand faces, except those that
    have their remote IP address in the 10.0.0.0/8 network.
