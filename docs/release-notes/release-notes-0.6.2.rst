NFD version 0.6.2
-----------------

Release date: May 4, 2018

Note that this is the last release that process packets with [NDN packet format version
0.2.1](https://named-data.net/doc/NDN-packet-spec/0.2.1/) semantics. A future release will
continue to decode v0.2.1 format, but will process packets with
[v0.3](https://named-data.net/doc/NDN-packet-spec/0.3/) semantics.

New features:
^^^^^^^^^^^^^

- ``afterContentStoreHit``, ``afterReceiveData`` strategy trigger (:issue:`4290`)

- ``Cs::erase`` method and ``cs/erase`` management command (:issue:`4318`)

- ``nfdc cs config`` to configure parameters of NFD Content Store at run time (:issue:`4050`)

- Support for IPv6 subnets in white-/blacklists of NFD config file (:issue:`4546`)

- Configurable IP subnets for "local" TCP faces (:issue:`4546`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Replace PIT straggler timer with a per-strategy decision (:issue:`4200`)

- Add ``isSatisfied`` and ``dataFreshnessPeriod`` properties to PIT entries
  (:issue:`4290`)

- Prevent potential nullptr dereference in ``FaceManager`` (:issue:`4548`)

- Enforce ``NameTree`` maximum depth universally (:issue:`4262`)

- Correctly handle removed fragments in ``LpReliability`` (:issue:`4479`)

- More hints, aliases, and visual improvements in ``nfdc`` command (:issue:`4498`)

- Fix listing of face flags in the HTML status page produced by ``nfd-status-http-server``

- Upgrade build environment to latest version of ``waf`` and other improvements

Removals:
~~~~~~~~~

- ``onInterestUnsatisfied`` pipeline stage (:issue:`4290`)
