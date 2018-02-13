NFD version 0.6.1
-----------------

Release date: February 19, 2018

New features:
^^^^^^^^^^^^^

- (potentially breaking change) ``nfd-status-http-server`` now requires Python 3

- Congestion detection and signaling for TCP, UDP, and Unix stream transports. This feature
  can be managed through the configuration file and nfdc, and is enabled by default
  (:issue:`4362`, :issue:`4465`)

- ``nfdc cs info`` command that shows CS hits and CS misses
  (:issue:`4219`, :issue:`4438`)

- Support for non-listening UDP channels (:issue:`4098`)

- IPv6 UDP multicast transport (:issue:`4222`)

- Strategy notification for packet drops in ``LpReliability`` (:issue:`3823`)

- ``NonDiscovery`` and ``PrefixAnnouncement`` encoding/decoding in ``GenericLinkService``
  (:issue:`4280`, :issue:`4355`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Added more ways in ``nfdc`` for a user to ask for help, including ``'nfdc foo help'``, ``'nfdc foo
  --help'``, and ``'nfdc foo -h'`` (:issue:`4503`)

- Added privilege elevation in ``MulticastUdpTransport`` that was preventing NFD from running with
  effective non-root user (:issue:`4507`)

- Fixed crash when configuration file lacks an ``'authorizations'`` section
  (:issue:`4487`)

- Made the exit status consistent across all tools

- Fixed build when ``std::to_string`` is not available
  (:issue:`4393`)

- Made ``AsfStrategy`` less sensitive, switching the path only after multiple timeouts. The number
  of timeouts is configurable via a strategy parameter (:issue:`4193`)

- Introduce depth limit in the Measurements table, limit the accepted prefix length for RIB,
  FIB, and StrategyChoice management commands (:issue:`4262`)

- Improved test cases and documentation
