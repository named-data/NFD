NFD version 0.3.4
-----------------

Release date: August 31, 2015

Changes since version 0.3.3:

New features:
^^^^^^^^^^^^^

- Boolean Face property IsOnDemand has been replaced with multistate FacePersistency
  property (:issue:`2989`, :issue:`3018`): persistent (Face exists as long as there are no
  system or socket errors), on-demand (Face is removed after inactivity period), and
  permanent (Face exists unless explicitly destroyed by the management).

  Currently, only UDP faces are allowed to be permanent.  However, management interface to
  create UDP permanent faces or switch UDP to permanent state will be available in the
  next release (:issue:`2991`).

- Least Recently Used policy for Content Store (:issue:`2219`)

  Currently, changing the policy requires a minor change in NFD source code.
  Configuration interface is coming in the next release (:issue:`3148`)


Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- Update of NDN Essential Tools to version 0.2 `<https://github.com/named-data/ndn-tools>`__
  with code improvements and the following new tools:

  * PIB service to manage the public information of keys and publish certificates
    (:issue:`3018`)
  * A Wireshark dissector for NDN packets (:issue:`3092`)

- RibManager constructor now accepts KeyChain as a parameter, preparing it for simplifying
  implementation with the new ``ndn::Dispatcher`` class (:issue:`2390`)

- Fix HTML rendering of nfd-status-http-server output in some browsers (:issue:`3019`)

- Enable automatic re-creation of multicast faces when network connectivity changes
  (:issue:`2460`)

- Enhance exception throwing with Boost Exception library (:issue:`2541`)

Deprecated:
^^^^^^^^^^^

- BroadcastStrategy (``/localhost/nfd/strategy/broadcast``) renamed as MulticastStrategy
  (``/localhost/nfd/strategy/multicast``) (:issue:`3011`)

Upcoming features (partially finished in development branches):
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- NDNLPv2 (http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2, :issue:`2520`,
  :issue:`2879`, :issue:`2763`, :issue:`2883`, :issue:`2841`, :issue:`2866`)

- Refactored implementation of NFD management (:issue:`2200`, :issue:`2107`)
