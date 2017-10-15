NFD version 0.6.0
-----------------

Release date: October 16, 2017

Changes since version 0.5.1:

New features
^^^^^^^^^^^^

- Generic prefix readvertise capability with specific support of readvertise-to-NLSR
  (:issue:`3818`)

- Unicast Ethernet faces, including close-on-idle, persistency change,
  fragmentation and reassembly features (:issue:`4011`, :issue:`4012`)

- Capability to create permanent TCP faces (:issue:`3167`)

- Configuration option to make UDP multicast and Ethernet multicast faces "ad hoc" (:issue:`4018`,
  :issue:`3967`)

- Replace ``Link`` with ``ForwardingHint`` as part of Interest processing (:issue:`4055`)

- ``ForwardingHint`` stripped when Interest reaches producer region (:issue:`3893`)

- New capabilities in multicast strategy:

  * Nack support (:issue:`3176`)
  * Consumer retransmission (:issue:`2062`)
  * Per-upstream exponential suppression of retransmissions (:issue:`4066`)

- Wildcard matching on interface names for whitelist/blacklist (:issue:`4009`)

- Optional best-effort link-layer reliability feature in NDNLPv2 (:issue:`3931`)

- Support for ``LpReliability`` flag in faces/create and faces/update (:issue:`4003`)

- Advisory limit of ``NameTree`` depth (:issue:`4262`)

- `Contributing guide <https://github.com/named-data/NFD/>`__ and `code of conduct
  <https://github.com/named-data/NFD/>`__ (:issue:`3898`)

Updates
^^^^^^^

- Allow forwarding of Interest/Data to ad hoc face regardless if the Interest/Data came from
  the same face (:issue:`3968`)

- Interpret omitted ``FreshnessPeriod`` as "always stale" (:issue:`3944`)

- Duplicate nonce from same face is no longer considered "loop" (:issue:`3888`)

- Refactor :doc:`nfdc command-line tool <../manpages/nfdc>` (:issue:`3864`, :issue:`3866`)

- Accept ``LocalUri`` in ``ProtocolFactory`` and ``FaceManager`` (:issue:`4016`)

- Abstract ``Nack`` processing out of ``BestRouteStrategy2`` (:issue:`3176`)

- Rework ``FacePersistency`` handling in faces/create and faces/update (:issue:`3232`)

  * Enable changing persistency in faces/update command
  * Reject faces/create request if the face already exists

- RIB updates to follow API changes in ndn-cxx library (:issue:`3903`)

- Switch to V2 security framework (:issue:`4089`)

- Use ``NetworkMonitor``, ``NetworkInterface``, ``lp::isLessSevere`` from ndn-cxx
  (:issue:`4021`, :issue:`4228`)

- Move ``Channel`` and subclasses into ``nfd::face`` namespace

- Improve consistency of logging in channels (:issue:`2561`)

- Change ``ndn-autoconfig`` tool to register ``/`` prefix instead of ``/ndn`` (:issue:`4201`)

- Documentation improvements

Bugfixes
^^^^^^^^

- In ASF strategy add check for FaceInfo existence before using it (:issue:`3968`)

- Avoid setting TransportState to FAILED if connection is closed cleanly (:issue:`4099`)

- Fix regression ``ndn-autoconfig`` continue proceeding with existing face (:issue:`4008`)

- Decode ``CachePolicy`` without requiring ``allowLocalFields`` (:issue:`3966`)

- Fix support for link-local IPv6 addresses (:issue:`1428`)

- Fix potential misaligned memory accesses (:issue:`4191`)

Deletions
^^^^^^^^^

- Deprecated code, including ``faces/enable-local-control`` and ``faces/disable-local-control``
  management commands (:issue:`3988`)

- ``NetworkInterfaceInfo`` class, replaced by ``ndn::net::NetworkInterface`` from ndn-cxx
  (:issue:`4021`)

- Legacy nfdc and nfd-status invocations

  The following legacy nfdc sub-commands are deleted, use the corresponding ``nfdc face ...``,
  ``nfdc route ...``, ``nfdc strategy ...`` commands:

  * ``register``
  * ``unregister``
  * ``create``
  * ``destroy``
  * ``set-strategy``
  * ``unset-strategy``
  * ``add-nexthop``
  * ``remove nexthop``

- ``nfd-status`` no longer accepts command line arguments (:issue:`4198`).  Individual datasets
  can be requested using ``nfdc channel list``, ``nfdc face list``, ``nfdc fib list``, ``nfdc
  route list``, and ``nfdc strategy list`` commands.

- ``nfdId`` from ``nfdc status`` output (:issue:`4089`)

- Prohibited endpoint set, making it possible to create faces that connect NFD to itself
  (:issue:`4189`)
