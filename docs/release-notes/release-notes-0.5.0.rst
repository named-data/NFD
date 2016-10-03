NFD version 0.5.0
-----------------

Release date: October 4, 2016

.. note::
   Version 0.5.0 introduces several breaking changes to the internal API (forwarding pipelines,
   strategy interface, tables) and wire format of management protocol

.. note::
   As of version 0.5.0, NFD requires a modern compiler (gcc >= 4.8.2, clang >= 3.4) and a
   relatively new version of Boost libraries (>= 1.54).  This means that the code no longer compiles
   with the packaged version of gcc and boost libraries on Ubuntu 12.04.  NFD can still be
   compiled on such systems, but requires separate installation of a newer version of the compiler
   (e.g., clang-3.4) and dependencies.

Changes since version 0.4.1

New features
^^^^^^^^^^^^

- Add Adaptive SRTT-based Forwarding strategy (:issue:`3566`)

- **breaking change** Introduce configurable policy for admission of unsolicited data packets into
  the content store (:issue:`2181`).  Currently available policies:

  * ``DropAllUnsolicitedDataPolicy`` (**the new default**): drop all unsolicited data packets
  * ``AdmitLocalUnsolicitedDataPolicy`` (the old default): allow unsolicited data packets from local
    applications to be cached (e.g., with a lower priority), drop all other unsolicited data
  * ``AdmitNetworkUnsolicitedDataPolicy``: allow unsolicited data packets from the network to be
    cached (e.g., with a lower priority), drop all other unsolicited data
  * ``AdmitAllUnsolicitedDataPolicy``: cache all unsolicited data packets

- Introduce mechanism to update properties (e.g., flags, persistency) of an existing Face
  (:issue:`3731`).  Note that the corresponding ``nfdc`` command will be available in the next
  release.

Updates
^^^^^^^

- **breaking change** Strategy API update. FIB entry is no longer supplied to the
  ``Strategy::afterReceiveInterest`` method (i.e., FIB lookup is not performed by the forwarding
  pipelines).  When necessary, a strategy can request FIB lookup using ``Strategy::lookupFib``
  (:issue:`3664`, :issue:`3205`, :issue:`3679`, :issue:`3205`)

- **breaking change** ForwarderStatus dataset can now be requested only with
  ``/localhost/nfd/status/general`` interest (:issue:`3379`)

- Optimizations of tables and forwarding, including reduced usage of ``shared_ptr`` (:issue:`3205`,
  :issue:`3164`, :issue:`3687`)

- Display extended diagnostic information if NFD crashes (:issue:`2541`)

- Visualize NACK counters in ``nfd-status`` output (:issue:`3569`)

- Extend management to process the new ``LocalFieldsEnabled`` attribute when creating/updating Faces
  (:issue:`3731`)

- Switch logging facility to use Boost.Log (:issue:`3562`)

- Refactor implementation of ``nfdc`` tool, which now supports a new command-line syntax and
  retrieval of status datasets (:issue:`3749`, :issue:`3780`).  This is the first step in
  implementing an interactive mode for ``nfdc`` (:issue:`2542`).

- ``nfd-status`` tool has been merged into ``nfdc`` with a wrapper script provided for backwards
  compatibility (:issue:`3658`)

- Refactor implementation of RIB Manager to make it uniform with other managers (:issue:`2857`)

- Miscellaneous code refactoring (:issue:`3738`, :issue:`3164`, :issue:`3687`, :issue:`3205`,
  :issue:`3608`, :issue:`3619`, :issue:`2181`)

- Update WebSocket++ to version 0.7.0 (:issue:`3588`)

- Updates to reflect the latest changes in ndn-cxx library (:issue:`3760`, :issue:`3739`,
  :issue:`2950`, :issue:`2063`)

Bugfixes
^^^^^^^^

- Ensure ``NccStrategy`` explores all potential upstreams (:issue:`3411`)

- Add missing processing of NACK in ``pit::Entry::hasUnexpiredOutRecords`` (:issue:`3545`)

- Fix issue with WebSocket-based Face creation when IPv4-mapped IPv6 loopback addresses are
  considered non-local (:issue:`3682`)

- Make sure that the outgoing Interest pipeline uses the newest in-record when sending out an
  Interest (:issue:`3642`)

- Properly delete PIT in-record and out-record when face is destroyed (:issue:`3685`)

- Fix ``Pit::find`` leak of ``NameTreeEntry`` (:issue:`3619`)

- Fix ``Pit::erase`` crash when Interest name contains implicit digest (:issue:`3608`)

- Fix use-after-free in ``Rib::erase`` and ``RibManagerFixture::clearRib`` (:issue:`3787`)

Deprecations
^^^^^^^^^^^^

- ``ClientControl`` forwarding strategy.  The NextHopFaceId is now honored universally
  (:issue:`3783`)

- ``StrategyInfoHost::getOrCreateStrategyInfo``, which is renamed to
  ``StrategyInfoHost::insertStrategyInfo`` (:issue:`3205`)

Deletions
^^^^^^^^^

- Previously deprecated BroadcastStrategy (:issue:`3206`)

- Unused command-line tool ``nrd`` (:issue:`3570`)

- ``SegmentPublisher`` and ``RibStatusPublisher``, both replaced by ``ndn::Dispatcher``
  (:issue:`2857`)

- ``CommandValidator``, which has been replaced by ``CommandAuthenticator`` (:issue:`2063`)
