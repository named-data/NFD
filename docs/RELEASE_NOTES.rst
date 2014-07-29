.. _NFD Release Notes:

NFD Release Notes
=================

NFD version 0.2.0 (changes since version 0.1.0)
-----------------------------------------------

Release date: August 25, 2014

- **Documentation**

  + `"NFD Developer's Guide" by NFD authors
    <http://named-data.net/wp-content/uploads/2014/07/NFD-developer-guide.pdf>`_ that
    explains NFD's internals including the overall design, major modules, their
    implementation, and their interactions

  + New detailed instructions on how to enable auto-start of NFD using OSX ``launchd``
    and Ubuntu's ``upstart`` (see `contrib/ folder
    <https://github.com/named-data/NFD/tree/master/contrib>`_)

- **Core**

  + Add support for temporary privilege drop and elevation (`Issue #1370
    <http://redmine.named-data.net/issues/1370>`_)

  + Add support to reinitialize multicast Faces and (partially) reload config file
    (`Issue #1584 <http://redmine.named-data.net/issues/1584>`_)

  + Randomization routines are now uniform across all NFD modules
    (`Issue #1369 <http://redmine.named-data.net/issues/1369>`_)

  + Enable use of new NDN naming conventions
    (`Issue #1837 <http://redmine.named-data.net/issues/1837>`_ and
    `#1838 <http://redmine.named-data.net/issues/1838>`_)

- **Faces**

  + `WebSocket <http://tools.ietf.org/html/rfc6455>`_ Face support
    (`Issue #1468 <http://redmine.named-data.net/issues/1468>`_)

  + Fix Ethernet Face support on Linux with ``libpcap`` version >=1.5.0
    (`Issue #1511 <http://redmine.named-data.net/issues/1511>`_)

  + Fix to recognize IPv4-mapped IPv6 addresses in ``FaceUri``
    (`Issue #1635 <http://redmine.named-data.net/issues/1635>`_)

  + Fix to avoid multiple onFail events
    (`Issue #1497 <http://redmine.named-data.net/issues/1497>`_)

  + Fix broken support of multicast UDP Faces on OSX
    (`Issue #1668 <http://redmine.named-data.net/issues/1668>`_)

  + On Linux, path MTU discovery on unicast UDPv4 faces is now disabled
    (`Issue #1651 <http://redmine.named-data.net/issues/1651>`_)

  + Added link layer byte counts in FaceCounters
    (`Issue #1729 <http://redmine.named-data.net/issues/1729>`_)

  + Face IDs 0-255 are now reserved for internal NFD use
    (`Issue #1620 <http://redmine.named-data.net/issues/1620>`_)

  + Serialized StreamFace::send(Interest|Data) operations using queue
    (`Issue #1777 <http://redmine.named-data.net/issues/1777>`_)

- **Forwarding**

  + Outgoing Interest pipeline now allows strategies to request a fresh ``Nonce``
    (e.g., when the strategy needs to re-express the Interest)
    (`Issue #1596 <http://redmine.named-data.net/issues/1596>`_)

  + Fix in the incoming Data pipeline to avoid sending packets to the incoming Face
    (`Issue #1556 <http://redmine.named-data.net/issues/1556>`_)

  + New ``RttEstimator`` class that implements the Mean-Deviation RTT estimator to be used
    in forwarding strategies

  + Fix memory leak caused by not removing PIT entry when Interest matches CS
    (`Issue #1882 <http://redmine.named-data.net/issues/1882>`_)

  + Fix spurious assertion in NCC strategy
    (`Issue #1853 <http://redmine.named-data.net/issues/1853>`_)

- **Tables**

  + Fix in ContentStore to properly adjust internal structure when ``Cs::setLimit`` is called
    (`Issue #1646 <http://redmine.named-data.net/issues/1646>`_)

  + New option in configuration file to set an upper bound on ContentStore size
    (`Issue #1623 <http://redmine.named-data.net/issues/1623>`_)

  + Fix to prevent infinite lifetime of Measurement entries
    (`Issue #1665 <http://redmine.named-data.net/issues/1665>`_)

  + Introducing capacity limit in PIT NonceList
    (`Issue #1770 <http://redmine.named-data.net/issues/1770>`_)

  + Fix memory leak in NameTree
    (`Issue #1803 <http://redmine.named-data.net/issues/1803>`_)

  + Fix segfault during Fib::removeNextHopFromAllEntries
    (`Issue #1816 <http://redmine.named-data.net/issues/1816>`_)

- **Management**

  + RibManager now fully support ``CHILD_INHERIT`` and ``CAPTURE`` flags
    (`Issue #1325 <http://redmine.named-data.net/issues/1325>`_)

  + Fix in ``FaceManager`` to respond with canonical form of Face URI for Face creation
    command (`Issue #1619 <http://redmine.named-data.net/issues/1619>`_)

  + Fix to prevent creation of duplicate TCP/UDP Faces due to async calls
    (`Issue #1680 <http://redmine.named-data.net/issues/1680>`_)

  + Fix to properly handle optional ExpirationPeriod in RibRegister command
    (`Issue #1772 <http://redmine.named-data.net/issues/1772>`_)

  + Added functionality of publishing RIB status (RIB dataset) by RibManager
    (`Issue #1662 <http://redmine.named-data.net/issues/1662>`_)

  + Fix issue of not properly canceling route expiration during processing of
    ``unregister`` command
    (`Issue #1902 <http://redmine.named-data.net/issues/1902>`_)

  + Enable periodic clean up of route entries that refer to non-existing faces
    (`Issue #1875 <http://redmine.named-data.net/issues/1875>`_)

- **Tools**

  + Extended functionality of ``nfd-status``

     * ``-x`` to output in XML format, see :ref:`nfd-status xml schema`
     * ``-c`` to retrieve channel status information (enabled by default)
     * ``-s`` to retrieve configured strategy choice for NDN namespaces (enabled by default)
     * Face status now includes reporting of Face flags (``local`` and ``on-demand``)
     * On-demand UDP Faces now report remaining lifetime (``expirationPeriod``)
     * ``-r`` to retrieve RIB information

  + Improved ``nfd-status-http-server``

     * HTTP server now presents status as XSL-formatted XML page
     * XML dataset and formatted page now include certificate name of the corresponding NFD
       (`Issue #1807 <http://redmine.named-data.net/issues/1807>`_)

  + Several fixes in ``ndn-autoconfig`` tool
    (`Issue #1595 <http://redmine.named-data.net/issues/1595>`_)

  + Extended options in ``nfdc``:

    * ``-e`` to set expiration time for registered routes
    * ``-o`` to specify origin for registration and unregistration commands

  + Enable ``all-faces-prefix'' option in ``nfd-autoreg`` to register prefix for all face
    (on-demand and non-on-demand)
    (`Issue #1861 <http://redmine.named-data.net/issues/1861>`_)

  + Enable processing auto-registration in ``nfd-autoreg`` for faces that existed
    prior to start of the tool
    (`Issue #1863 <http://redmine.named-data.net/issues/1863>`_)

- **Build**

  + Enable support of precompiled headers for clang and gcc to speed up compilation

- `Other small fixes and extensions
  <https://github.com/named-data/NFD/compare/NFD-0.1.0...master>`_

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
------------------------------------------------

- NACK
    A packet sent back by a producer or a router to signal the unavailability of a requested
    Data packet. The protocol specification for NACK is in progress.

- New strategies
    Additional strategies, including self-learning that populates the FIB by observing
    Interest and Data exchange.

- Hop-by-hop Interest limit mechanism
    For congestion control

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
    Configurable organization-based scoping
