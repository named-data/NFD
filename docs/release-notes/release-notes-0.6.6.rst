NFD version 0.6.6
-----------------

Release date: April 29, 2019

Note that this is the last release that encodes to `NDN packet format version 0.2.1
<https://named-data.net/doc/NDN-packet-spec/0.2.1/>`__. A future release will continue to
decode v0.2.1 format, but will encode to `v0.3 format
<https://named-data.net/doc/NDN-packet-spec/0.3/>`__.

New features
^^^^^^^^^^^^

- Initial code changes for self-learning for broadcast and ad hoc wireless faces
  (:issue:`4281`)

  * ``EndpointId`` field in PIT in-record and out-record (:issue:`4842`)

  * ``FaceEndpoint`` parameter in Forwarding and Strategy API (:issue:`4849`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Fix to properly handle consumer retransmission in AsfStrategy (:issue:`4874`)

- Fix compilation error when dropping privileges is not supported, e.g., on Android platform
  (:issue:`4833`)

- Replace all uses of ``BOOST_THROW_EXCEPTION`` with ``NDN_THROW``, and custom
  ``nfd::getExtendedErrorMessage`` with standard ``boost::diagnostic_information`` for error
  message formatting (:issue:`4834`)

- Fix compilation against recent versions of Boost libraries (:issue:`4890`, :issue:`4923`)

- Fix display of satisfied/unsatisfied Interests in ``nfd-status-http-server`` (:issue:`4720`)

- Move ``NFD_VERSION{,_BUILD}_STRING`` to ``version.cpp`` to avoid re-compilation when the git
  commit hash changes

- Code optimizations, modernizations, and deduplications:

  * Switch to use ndn-cxx's ``getRandomNumberEngine()``

  * Switch to ``std::thread`` and ``thread_local``

- Code reorganization:

  * Move entire ``rib`` subdir to ``daemon/rib``, except for ``RibManager``, which is moved to
    ``daemon/mgmt`` (:issue:`4528`)

  * Move NFD-specific files from ``core/`` to ``daemon/`` folder (:issue:`4922`)

  * Eliminate ``scheduler::{schedule,cancel}`` wrappers

  * Merge ``ManagerBase`` with ``NfdManagerBase`` (:issue:`4528`)

  * Introduce and make use of ``NullLinkService`` and ``NullTransport`` (:issue:`4528`)

- Test suite refactoring

  * ``IdentityManagementFixture`` renamed to ``KeyChainFixture`` and no longer derives from
    ``BaseFixture``

  * Introduce ``ClockFixture``

  * ``unit-tests-{core,tools}`` no longer require a global ``io_service``

  * Eliminate use of ``Selectors`` in CS tests (:issue:`4805`)

- Update documentation

Removals
^^^^^^^^

- Support for ``unit-tests.conf`` (use ``NDN_LOG`` to customize)
