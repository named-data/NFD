NFD version 0.4.0
-----------------

Release date: December 31, 2015

.. note::
   Version 0.4.0 introduces several breaking changes to API and wire format of management protocols

Changes since version 0.3.4:

New features:
^^^^^^^^^^^^^

- **(breaking change)** Refactored implementation of face system (:issue:`3088`, :issue:`3104`,
  :issue:`3165`, :issue:`3168`, :issue:`3225`, :issue:`3226`, :issue:`3253`, :issue:`3259`,
  :issue:`3169`, :issue:`3160`, :issue:`3278`, :issue:`3166`, :issue:`3177`, :issue:`3177`,
  :issue:`3172`)

  The abstraction to send/receive NDN packets has been split into Transport, LinkService, and Face:

  * *Transport* provides delivery of the data blocks over specific underlying channels
    (raw ethernet packets, unicast/multicast UDP datagrams, TCP and WebSocket streams)

    Implemented: :NFD:`UnixStreamTransport`, :NFD:`EthernetTransport`, :NFD:`UnicastUdpTransport`,
    :NFD:`MulticastUdpTransport`, :NFD:`WebSocketTransport`, :NFD:`TcpTransport`,
    :NFD:`InternalForwarderTransport`

  * *LinkService* provides an "adaptation" layer to translate between NDN packets and data
    blocks communicated through Transport.  For example, LinkService can provide packet
    fragmentation and reassembly services.

    Implemented: :NFD:`GenericLinkService`

  * *Face* provides combines Transport and LinkServices, providing high-level interface to work
    with Interest/Data/Nack packets inside NFD.

  .. note::
     This feature replaces support for signaling between local applications and NFD from
     LocalControlHeader with NDNLPv2 protocol.  If your application uses
     LocalControlHeader features, it must be updated to use the new protocol (e.g., update
     to use ndn-cxx >= 0.4.0)

- Support for NDNLPv2 (http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2,
  :issue:`2520`, :issue:`2763`, :issue:`2841`)

  * Network NACK (:issue:`2883`)
  * Fragmentation and reassembly  (:issue:`3171`)

- Networking NACK in pipelines and best-route strategy (:issue:`3156`)

- Refactored implementation of NFD management (:issue:`2200`, :issue:`2107`)

- Interest forwarding processes Link included in interest packets (:issue:`3034`)

  .. note::
     This feature requires proper defintion of new ``tables.network_region`` section in
     the NFD config file (:issue:`3159`)

- Full support for UDP permanent faces (:issue:`2993`, :issue:`2989`, :issue:`3018`)

- Automatic start of NDN auto-configuration client (``ndn-autoconfig``) when starting NFD
  using ``nfd-start``, when enabled in ``autoconfig.conf`` (:issue:`2716`)

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- **(breaking change)** Redesign of automatic prefix propagation, formerly known as remote
  prefix registration (:issue:`3211`, :issue:`2413`)

  This includes a backward-incompatible change to NFD's configuration file:

  * ``rib.remote_register`` section has been removed and, if present, will cause failure for NFD to start
  * ``rib.auto_prefix_propagate`` section has been added to control automatic prefix propagation feature

- Adapt pcap filter in EthernetTransport to accept only untagged frames (:issue:`3348`)

- Increase the initial suppression interval in BestRouteStrategy2 to 250ms (:issue:`3230`)

- Fix potential crash in ``Measurements::get(Fib::s_emptyEntry)`` (:issue:`3275`)

- Fix memory leak in PriorityFifoPolicy (:issue:`3236`)

- Display extended information for fatal NFD errors (:issue:`2541`)

- Compilation fixes for clang-700.0.72 (Apple LLVM 7.0.0) (:issue:`3209`)

- Properly handle exception from NetworkMonitor when the platform doesn't support it
  (:issue:`3195`)

- Multiple test suite improvements (:issue:`3322`, :issue:`3306`, :issue:`3305`, :issue:`3307`,
  :issue:`3346`, :issue:`3327`)

Deprecated:
^^^^^^^^^^^

- BroadcastStrategy (``/localhost/nfd/strategy/broadcast``) renamed as MulticastStrategy
  (``/localhost/nfd/strategy/multicast``) (:issue:`3011`)

- ForwarderStatus dataset retrievable using ``/localhost/nfd/status`` name, use
  ``/localhost/nfd/status/general`` instead.

Deleted:
^^^^^^^^

- NotificationStream, replaced by the version in ndn-cxx library (:issue:`2144`)

Known issues:
^^^^^^^^^^^^^

- NFD currently has a known limitation in supporting the retrieval of data using full names,
  i.e., names with the implicit digest (:issue:`3363`).  This limitation will be addressed
  in the next release.
