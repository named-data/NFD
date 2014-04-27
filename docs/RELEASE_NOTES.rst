.. _NFD v0.1.0 Release Notes:

NFD v0.1.0 Release Notes
========================

Major Modules of NFD
--------------------

NFD has the following major modules:

- Core
    Provides various common services shared between different NFD modules. These include
    hash computation routines, DNS resolver, config file, face monitoring, and
    several other modules.

- Faces
    Implements the NDN face abstraction on top of different lower level transport
    mechanisms.

- Tables
    Implements the Content Store (CS), the Pending Interest Table (PIT), the Forwarding
    Information Base (FIB), and other data structures to support forwarding of NDN Data
    and Interest packets.

- Forwarding
    Implements basic packet processing pathways, which interact with Faces, Tables,
    and Strategies.

  + **Strategy Support**, a major part of the forwarding module
      Implements a framework to support different forwarding strategies. It includes
      StrategyChoice, Measurements, Strategies, and hooks in the forwarding pipelines. The
      StrategyChoice records the choice of the strategy for a namespace, and Measurement
      records are used by strategies to store past performance results for namespaces.

- Management
    Implements the `NFD Management Protocol
    <http://redmine.named-data.net/projects/nfd/wiki/Management>`_, which allows
    applications to configure NFD and set/query NFD's internal states.  Protocol interaction
    is done via NDN's Interest/Data exchange between applications and NFD.

- RIB Management
    Manages the routing information base (RIB).  The RIB may be updated by different parties
    in different ways, including various routing protocols, application's prefix
    registrations, and command-line manipulation by sysadmins.  The RIB management module
    processes all these requests to generate a consistent forwarding table, and then syncs
    it up with the NFD's FIB, which contains only the minimal information needed for
    forwarding decisions. Strictly speaking RIB management is part of the NFD management
    module. However, due to its importance to the overall operations and its more complex
    processing, we make it a separate module.

Features in Version 0.1.0
-------------------------

This is an incomplete list of features that are implemented in NFD version 0.1.0.

- Packet Format

  + `NDN-TLV <http://named-data.net/doc/ndn-tlv/>`_
  + LocalControlHeader, to allow apps to set outgoing face and learn incoming face.

- Faces

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

- Management

  + Use of signed Interests as commands, with authentication and authorization.
  + Face management
  + FIB management
  + Per-namespace strategy selection
  + NFD status publishing
  + Notification to authorized apps of internal events, including Face creation and destruction.

- Tables and forwarding pipelines support most Interest/Data processing, including
  selectors.

- RIB Management that runs as a separate process, ``nrd``.  It supports basic prefix
  registration by applications, but no flags yet.

- Strategies

  + ``broadcast``
  + ``best-route``
  + ``ncc``: based on ccnx 0.7 for experimentation
  + ``client-control``: authorized application can directly control Interest forwarding

- Name-based scoping

  + ``/localhost``: communication only within localhost using "local" Faces
    (UnixStreamFace, LocalTcpFace).  NFD will strictly enforce this scope for Interests
    and Data packets
  + ``/localhop``: one-hop communication (e.g., if at least one incoming or outgoing Face
    in PIT entry is non-local, the Interest cannot be forwarded to any non-local Face)

- Support configuration file, which is in the Boost INFO format.

- Applications

  + Tools to discover hubs on NDN testbed.
  + peek/poke and traffic generators for testing and debugging.
  + ``nfdc``, a command-line tool to configure NFD.
  + ``nfd-status``, a command-line tool to query NFD status.
  + ``nfd-status-http-server``, which reads the NFD status and publishes over HTTP.


Planned Functions and Features for Next Releases
------------------------------------------------

- NACK
    A packet sent back by a producer or a router to signal the unavailability of a requested
    Data packet. The protocol specification for NACK is in progress.

- New strategies
    Additional strategies, including self-learning that populates the FIB by observing
    Interest and Data exchange.

- Hop-by-hop Interest limit mechanism for congestion control.

- Face enhancements
    Add fragmentation support for Ethernet face, may add support for new types such as
    WiFi direct and WebSockets.

- Tables
    Experiment and evaluate different data structures and algorithms.

- RIB management
    Move to more scalable data structures and support all flags in prefix registrations.

- Tunnel management
    For hub nodes to authenticate incoming tunnel requests and maintain the tunnels.

- Extensible name-based scoping

  + configurable organization-based scoping
