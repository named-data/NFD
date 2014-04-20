ndn-autoreg
===========

Usage
-----

::

    nfd-autoreg --prefix=</autoreg/prefix> [--prefix=</another/prefix>] ...

Description
-----------

``autoreg-server`` is a deamon application that automatically registers the specified
prefix(es) when new on-demand Face is created (i.e., when a new UDP face is created
as a result of an incoming packet or TCP face is created as a result of an incoming
connection).

Options
-------

``-h`` or ``--help``
  Produce help message.

``-i`` or ``--prefix``
  Prefix that should be automatically registered when a new remote face is created.
  Can be repeated multiple to specify additional prefixes.

``-c`` or ``--cost``
  RIB cost to be assigned to auto-registered prefixes.   if not specified, default cost
  is set to 255.

``-w`` or ``--whitelist``
  Whitelisted network, e.g., 192.168.2.0/24 or ::1/128.   Can be repeated multiple times
  to specify multiple whitelisted networks.

  Prefix(es) will be auto-registered only when remote IP address is within the specified
  range(s), except blacklist ranges.

  Default: 0.0.0.0/0 and ::/0

``-b`` or ``--blacklist``
  Blacklisted network, e.g., 192.168.2.32/30 or ::1/128.  Can be repeated multiple times
  to specify multiple blacklisted networks.

  Prefix(es) will be auto-registered only when remote IP address in **NOT** within the
  specified range(s), but is within the range define by the whitelist(s).

  Default: none

Examples
--------

Auto-register two prefixes for any newly created on-demand Face, except those that has
source IP address in ``10.0.0.0/8`` network::

    nfd-autoreg --prefix=/app1/video --prefix=/app2/pictures -b 10.0.0.0/8
