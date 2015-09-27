NFD version 0.1.0
-----------------

Release date: May 7, 2014

This is an incomplete list of features that are implemented in NFD version 0.1.0.

- **Packet Format**

  + `NDN-TLV <http://named-data.net/doc/ndn-tlv/>`_
  + LocalControlHeader, to allow apps to set outgoing face and learn incoming face.

- **Faces**

  + Unix stream socket
  + UDP unicast
  + UDP multicast
  + TCP
  + Ethernet, currently without fragmentation.

    .. note::
         Ethernet support will not work properly on Linux kernels with TPACKET_V3 flexible
         buffer implementation (>= 3.2.0) and libpcap >= 1.5.0 (e.g., Ubuntu Linux 14.04).
         Refer to `Issue 1551 <http://redmine.named-data.net/issues/1511>`_ for more
         detail and implementation progress.

- **Management**

  + Use of signed Interests as commands, with authentication and authorization.
  + Face management
  + FIB management
  + Per-namespace strategy selection
  + NFD status publishing
  + Notification to authorized apps of internal events, including Face creation and destruction.

- **Tables and forwarding pipelines** support most Interest/Data processing, including
  selectors.

- **RIB Management** that runs as a separate process, ``nrd``.  It supports basic prefix
  registration by applications, but no flags yet.

- **Strategies**

  + ``broadcast``
  + ``best-route``
  + ``ncc``: based on ccnx 0.7 for experimentation
  + ``client-control``: authorized application can directly control Interest forwarding

- **Name-based scoping**

  + ``/localhost``: communication only within localhost using "local" Faces
    (UnixStreamFace, LocalTcpFace).  NFD will strictly enforce this scope for Interests
    and Data packets
  + ``/localhop``: one-hop communication (e.g., if at least one incoming or outgoing Face
    in PIT entry is non-local, the Interest cannot be forwarded to any non-local Face)

- **Support configuration file**, which is in the Boost INFO format.

- **Applications**

  + Tools to discover hubs on NDN testbed.
  + peek/poke and traffic generators for testing and debugging.
  + ``nfdc``, a command-line tool to configure NFD.
  + ``nfd-status``, a command-line tool to query NFD status.
  + ``nfd-status-http-server``, which reads the NFD status and publishes over HTTP.


Planned Functions and Features for Next Releases
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- NACK
    A packet sent back by a producer or a router to signal the unavailability of a requested
    Data packet. The protocol specification for NACK is in progress.

- New strategies
    Additional strategies, including self-learning that populates the FIB by observing
    Interest and Data exchange.

- Hop-by-hop Interest limit mechanism
    For congestion control

- Face enhancements
    Add faces for new underlying protocols such as WiFi direct.
    Introduce the concept of "permanent faces" that can survive socket errors.
    Design a new hop-by-hop header that supports fragmentation, reliability improvement, etc.

- Tables
    Experiment and evaluate different data structures and algorithms.

- RIB management
    Move to more scalable data structures and support all flags in prefix registrations.

- Tunnel management
    For hub nodes to authenticate incoming tunnel requests and maintain the tunnels.

- Extensible name-based scoping
    Configurable organization-based scoping
