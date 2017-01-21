NFD version 0.5.1
-----------------

Release date: January 25, 2017

Changes since version 0.5.0

New features
^^^^^^^^^^^^

- In ndn-autoconfig, add the Find (geographically) Closest Hub stage (NDN-FCH) using a
  `deployed service <http://ndn-fch.named-data.net>`__ (:issue:`3800`)

- Add ``face_system.ether.whitelist`` and ``face_system.ether.blacklist`` config options for
  Ethernet multicast faces creation (:issue:`1712`)

- Add ``face_system.udp.whitelist`` and ``face_system.udp.blacklist`` config options for
  UDP multicast faces creation (:issue:`1712`)

- Introduce ``tables.cs_policy`` config option to configure cache policy (policy change requires
  restart of NFD) (:issue:`3148`)

- Introduce strategy parameters that can be specified when selecting a strategy for a
  namespace (:issue:`3868`)

- Initial implementation of route re-advertise feature.  This release only includes supporting
  classes.  Full implementation of this feature is expected to be introduced in the next
  release (:issue:`3818`).

- Introduce ``FACE_EVENT_UP`` and ``FACE_EVENT_DOWN`` notifications (:issue:`3794`, :issue:`3818`)

- GenericLinkService now encodes and decodes the NDNLPv2 CongestionMark field (:issue:`3797`)

Updates
^^^^^^^

- Allow strategies to pick outgoing Interest.  The outgoing Interest pipeline and Strategy API
  have changed to give strategies an opportunity to pick an outgoing Interest that matches
  the Interest table entry (:issue:`1756`)

- Refactor localhop restriction handling in strategies.  Previously available
  ``canForwardToLegacy`` function no longer checks scope. Strategies using this function shall
  use ``wouldViolateScope`` separately (:issue:`3841`, :issue:`1756`)

- Strategy instances are no longer shared among different StrategyChoice entries.  Each instance can
  be individually configured using the arguments supplied during construction (as part of strategy
  name) (:issue:`3868`)

- Change ``strategy-choice/set`` response codes.  Specifically, if the specified strategy name
  cannot be instantiated, the command responds with 404 status code (:issue:`3868`)

- Add ability to instantiate strategy of next higher version if the requested version is not
  available (:issue:`3868`)

- Refactor face management system.  The function of the monolithic ``FaceManager`` class has
  been split among the newly introduced ``FaceSystem`` class and the various protocol
  factories.  FaceSystem class is the entry point of NFD's face system and owns the concrete
  protocol factories, created based on ``face_system`` section of the NFD configuration
  file. (:issue:`3904`)

- Switch to use ``faces/update`` instead of legacy ``faces/enable-local-control`` to enable
  local fields (:issue:`3734`)

- Add support for permanent persistency in TcpTransport (:issue:`3167`)

- Add missing and refactor existing registry implementations for replaceable modules, including
  strategy, protocol factory, CS policy, and unsolicited data policy (:issue:`2181`, :issue:`3148`,
  :issue:`3868`, :issue:`3904`)

- Refactor and extend ``nfdc`` tool. The tool is now a universal instrument to query and
  change NFD state (:issue:`3780`)

- Refactor implementation of ``ndn-autoconfig`` tool (:issue:`2426`)

Bugfixes
^^^^^^^^

- Fix RTT calculation in ASF strategy (:issue:`3829`)

- Ensure that ``pit::Entry`` checks that Interest matches entry when updating in/out-record to
  prevent strategies to incorrectly pick an incorrect Interest (:issue:`1756`)

- Fix uncaught ``bad_lexical_cast`` exception in ``Network::isValidCidr()`` (:issue:`3858`)

- Fix incorrect output of ``operator<<`` for Rib class (:issue:`3423`)

- Make FIB and StrategyChoice iterators default-constructible (:issue:`3882`)

- Ensure that ``nfd-status-http-server`` returns error code (HTTP/504) when NFD is not running
  (:issue:`3863`)

- A number of fixes in documentation

Deprecations
^^^^^^^^^^^^

- Deprecate ``nfd-status`` command line options.  Use ``nfdc`` subcommands, such as ``nfdc face
  list`` and ``nfdc status report xml``. The argument-less ``nfd-status`` is retained as an
  alias of ``nfdc status report`` (:issue:`3780`)

Deletions
^^^^^^^^^

- Delete deprecated ``Strategy::sendInterest`` overload and ``violatesScope`` (:issue:`1756`,
  :issue:`3841`)

- StrategyChoice no longer supports installed instances.  All strategies should be registered
  in the strategy registry (:issue:`3868`)

- ``Strategy::getName``. Instead, Strategy API introduces ``getStrategyName`` (strategy program
  name, including version component) and ``getInstanceName`` (strategy name assigned during
  strategy instance instantiation, including version and optional extra parameter components)
  (:issue:`3868`)
