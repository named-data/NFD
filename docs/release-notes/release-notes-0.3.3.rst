NFD version 0.3.3
-----------------

Release date: July 1, 2015

Changes since version 0.3.2:

New features:
^^^^^^^^^^^^^

- Content Store replacement policy interface (:issue:`1207`)

- Add unit file and instructions for systemd (:issue:`1586`)

- NDN Essential Tools version 0.1 `<https://github.com/named-data/ndn-tools>`__ featuring:

  * ``ndnpeek``, ``ndnpoke``: a pair of programs to request and make available for retrieval of
    a single Data packet
  * ``ndnping``, ``ndnpingserver``: reachability testing tools for Named Data Networking
  * ``ndndump``: a traffic analysis tool that captures Interest and Data packets on the wire
  * ``ndn-dissect``: an NDN packet format inspector. It reads zero or more NDN packets from
    either an input file or the standard input, and displays the Type-Length-Value (TLV)
    structure of those packets on the standard output.

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- Avoid loopback new Interest in AccessStrategy (:issue:`2831`)

- Simplified implementation of ``nfd-status`` using SegmentFetcher utility class (:issue:`2456`)

Deprecated:
^^^^^^^^^^^

- ``ndn-tlv-peek`` and ``ndn-tlv-poke`` command-line tools: use ``ndnpeek`` and ``ndnpoke``
  programs from NDN Essential Tools repository `<https://github.com/named-data/ndn-tools>`__.

Upcoming features:
^^^^^^^^^^^^^^^^^^

- NDNLPv2 (http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2, :issue:`2520`,
  :issue:`2879`, :issue:`2763`, :issue:`2883`, :issue:`2841`, :issue:`2866`)

- Refactored implementation of NFD management (:issue:`2200`, :issue:`2107`)
