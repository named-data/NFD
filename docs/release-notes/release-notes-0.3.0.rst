NFD version 0.3.0
-----------------

Release date: February 2, 2015

Changes since version 0.2.0:

New features:
^^^^^^^^^^^^^

- **Build**

  + The code now requires C++11.  The minimum supported gcc version is 4.6, as earlier versions
    do not have proper support for C++11 features.

- **Faces**

  + Enable detection of WebSocket connection failures using ping/pong messages (:issue:`1903`)

  + In EthernetFace:

    * Avoid putting the NIC in promiscuous mode if possible (:issue:`1278`)

    * Report packets dropped by the kernel if debug is enabled (:issue:`2441`)

    * Integrate NDNLP fragmentation (:issue:`1209`)

- **Forwarding**

  + Strategy versioning (:issue:`1893`)

  + New Dead Nonce List table to supplement PIT for loop detection (:issue:`1953`)

  + Abstract retransmission suppression logic (:issue:`2377`)

  + New forwarding strategy for access router (:issue:`1999`)

- **Management**

  + Add config file-based strategy selection (:issue:`2053`)

    The sample config file now includes strategy selection for ``/``, ``/localhost``,
    ``/localhost/nfd``, and ``/ndn/broadcast`` namespaces as follows:

    ::

        tables
        {
          ...
          strategy_choice
          {
            /               /localhost/nfd/strategy/best-route
            /localhost      /localhost/nfd/strategy/broadcast
            /localhost/nfd  /localhost/nfd/strategy/best-route
            /ndn/broadcast  /localhost/nfd/strategy/broadcast
          }
        }

  + Implement Query Operation in FaceManager (:issue:`1993`)

  + FaceManager now responds with producer-generated NACK when query is invalid (:issue:`1993`)

  + Add functionality for automatic remote prefix registration (:issue:`2056`)

  + Only canonical FaceUri are allowed in faces/create commands (:issue:`1910`)

- **Tables**

  + StrategyInfoHost can now store multiple StrategyInfo of distinct types (:issue:`2240`)

  + Enable iteration over PIT and CS entries (:issue:`2339`)

  + Allow predicate to be specified in Measurements::findLongestPrefixMatch (:issue:`2314`)

  + Calculate the implicit digest of Data packets in CS only when necessary (:issue:`1706`)

- **Tools**

  + Publish ``/localhop/ndn-autoconf/routable-prefixes`` from ``ndn-autoconfig-server``
    (:issue:`1954`)

  + Display detailed NFD software verion in ``nfd-status-http-server`` and ``nfd-status``
    (:issue:`1916`)

  + ``nfdc`` now accepts FaceUri in all commands (:issue:`1995`)

  + Add daemon mode for ``ndn-autoconfig`` to re-run detection when connectivity changes
    (:issue:`2417`)

- **Core**

  + New scheduler::ScopedEventId class to automatically handle scheduled event lifetime
    (:issue:`2295`)

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- **Documentation**

  + NFD Developer's guide has been updated to reflect changes in the codebase

  + Installation instruction updates

  + Update of config file instructions for disabling unix sockets (:issue:`2190`)

- **Core**

  + Use implementations moved to ndn-cxx library

     + Use Signal from ndn-cxx (:issue:`2272`, :issue:`2300`)

     + use ethernet::Address from ndn-cxx (:issue:`2142`)

     + Use MAX_NDN_PACKET_SIZE constant from ndn-cxx (:issue:`2099`)

     + Use DEFAULT_INTEREST_LIFETIME from ndn-cxx (:issue:`2202`)

     + Use FaceUri from ndn-cxx (:issue:`2143`)

     + Use DummyClientFace from ndn-cxx (:issue:`2186`)

     + Use ndn::dns from ndn-cxx (:issue:`2207`)

  + Move Network class implementation from ``tools/`` to ``core/``

  + Ignore non-Ethernet ``AF_LINK`` addresses when enumerating NICs on OSX and other BSD systems

  + Fix bug on not properly setting FreshnessPeriod inside SegmentPublisher (:issue:`2438`)

- **Faces**

  + Fix spurious assertion failure in StreamFace (:issue:`1856`)

  + Update websocketpp submodule (:issue:`1903`)

  + Replace FaceFlags with individual fields (:issue:`1992`)

  + Drop WebSocket message if the size is larger than maximum NDN packet size (:issue:`2081`)

  + Make EthernetFace more robust against errors (:issue:`1984`)

  + Prevent potential infinite loop in TcpFactory and UdpFactory (:issue:`2292`)

  + Prevent crashes when attempting to create a UdpFace over a half-working connection
    (:issue:`2311`)

  + Support MTU larger than 1500 in EthernetFace (for jumbo frames) (:issue:`2305`)

  + Re-enable EthernetFace on OS X platform with boost >=1.57.0 (:issue:`1922`)

  + Fix ``ioctl()`` calls on platforms where libpcap uses ``/dev/bpf*`` (:issue:`2327`)

  + Fix overhead estimation in NDNLP slicer (:issue:`2317`)

  + Replace usage of deprecated EventEmitter with Signal in Face abstractions (:issue:`2300`)

  + Fix NDNLP PartialMessage cleanup scheduling (:issue:`2414`)

  + Remove unnecessary use of DNS resolver in (Udp|Tcp|WebSocket)Factory (:issue:`2422`)

- **Forwarding**

  + Updates related to NccStrategy

    * Fix to prevent remembering of suboptimal upstreams (:issue:`1961`)

    * Optimizing FwNccStrategy/FavorRespondingUpstream test case (:issue:`2037`)

    * Proper detection for new PIT entry (:issue:`1971`)

    * Use UnitTestTimeFixture in NCC test case (:issue:`2163`)

    * Fix loop back to sole downstream (:issue:`1998`)

  + Updates related to BestRoute strategy

    + Redesign best-route v2 strategy test case (:issue:`2126`)

    + Fix clang compilation error in best-route v2 test case (:issue:`2179`)

    + Use UnitTestClock in BestRouteStrategy2 test (:issue:`2160`)

  + Allow strategies limited access to FaceTable (:issue:`2272`)

- **Tables**

  + Ensure that eviction of unsolicited Data is done in FIFO order (:issue:`2043`)

  + Simplify table implementations with C++11 features (:issue:`2100`)

  + Fix issue with Fib::removeNextHopFromAllEntries invalidating NameTree iterator
    (:issue:`2177`)

  + Replace deprecated EventEmitter with Signal in FaceTable (:issue:`2272`)

  + Refactored implementation of ContentStore based on std::set (:issue:`2254`)

- **Management**

  + Allow omitted FaceId in faces/create command (:issue:`2031`)

  + Avoid deprecated ``ndn::nfd::Controller(Face&)`` constructor (:issue:`2039`)

  + Enable check of command length before accessing verb (:issue:`2151`)

  + Rename FaceEntry to Route (:issue:`2159`)

  + Insert RIB command prefixes into RIB (:issue:`2312`)

- **Tools**

  + Display face attribute fields instead of FaceFlags in ``nfd-status`` and
    ``nfd-status-http-server`` output (:issue:`1991`)

  + Fix ``nfd-status-http-server`` hanging when nfd-status output is >64k (:issue:`2121`)

  + Ensure that ``ndn-autoconfig`` canonizes FaceUri before sending commands to NFD
    (:issue:`2387`)

  + Refactored ndn-autoconfig implementation (:issue:`2421`)

  + ndn-autoconfig will now register also ``/localhop/nfd`` prefix towards the hub (:issue:`2416`)

- **Tests**

  + Use UnitTestClock in Forwarder persistent loop test case (:issue:`2162`)

  + Use LimitedIo in FwForwarder/SimpleExchange test case (:issue:`2161`)

- **Build**

  + Fix build error with python3 (:issue:`1302`)

  + Embed CI build and test running script

  + Properly disable assertions in release builds (:issue:`2139`)

  + Embed setting of ``PKG_CONFIG_PATH`` variable to commonly used values (:issue:`2178`)

  + Add conditional compilation for NetworkInterface and PrivilegeHelper

  + Support tools with multiple translation units (:issue:`2344`)

Removals
^^^^^^^^

- Remove ``listen`` option from unix channel configuration (:issue:`2188`)

- Remove usage of deprecated ``MetaInfo::TYPE_*`` constants (:issue:`2128`)

- Eliminate MapValueIterator in favor of ``boost::adaptors::map_values``
