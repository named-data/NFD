NFD version 0.3.1
-----------------

Release date: March 3, 2015

Changes since version 0.3.0:

New features:
^^^^^^^^^^^^^

- ``nfd`` and ``nrd`` daemons are now merged into a single process using separate threads
  (:issue:`2489`)

- **Core**

  - Extend ConfigFile to support passing a parsed ConfigSection (:issue:`2495`)

  - Allow customization of Logger and LoggerFactory (:issue:`2433`)

  - Make global io_service, scheduler, and global random generator thread-local, and logger
    thread-safe (:issue:`2489`)

- **Forwarding**

  - Introduce exponential back-off interest retransmission suppression mechanism and enable
    its use in the best-route strategy (:issue:`1913`)

  - Strategies are now registered with a macro, making it simpler to introduce new strategies
    to NFD codebase (:issue:`2410`)

- **Tables**

  - ContentStore now recognizes CachingPolicy-NoCache from LocalControlHeader (:issue:`2185`)

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- **Faces**

  - Remove Tcp|UdpChannel::connect overloads that perform DNS resolution (:issue:`2422`)

  - Properly handle error conditions in UdpChannel::newPeer (:issue:`2516`)

  - Fix inconsistency of UDP face timeouts in sample `nfd.conf` and actual defaults
    (:issue:`2473`)

  - Introduce Face-specific logging macros (:issue:`2450`)

  - Refactor handling of LinkType face trait and fix FaceStatus reporting: the link type was
    not properly propagated (:issue:`2563`)

  - Avoid exceptions in NDNLP PartialMessageStore (:issue:`2261`)

  - Update websocketpp to version 0.5.1

- **Tables**

  - Reduce priority of DeadNonceList log messages from DEBUG to TRACE

- **Management**

  - Change register/unregister logging in RibManager to INFO level (:issue:`2547`)

- **Tools**

  - Change prefix for :ref:`the local hub prefix discovery <local-prefix-discovery>` to be
    under ``/localhop/nfd`` (:issue:`2479`, :issue:`2512`)

- **Tests**

  - Change naming conventions for unit test files and test suite names (:issue:`2497`)

  - Fix segfault in TableNameTree test suite when all test logs are enabled (:issue:`2564`)
