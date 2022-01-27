NFD version 22.02
-----------------

Release date: February 17, 2022

Note that starting with this release, NFD switched to a new versioning convention:
``YEAR.MONTH[.REVISION]``.

Notable changes and new features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Remove NACKs from multicast strategy (:issue:`5146`)

- Dispatch pending interests when a new next hop is created in ``MulticastStrategy``
  (:issue:`4931`)

- Reorder function parameters in Strategy classes to make the API more uniform
  (:issue:`5173`)

- Add default ``HopLimit`` to Interests when missing (:issue:`5171`)

- Allow batch command processing by ``nfdc`` to accomodate case when ``nfdc`` is used to
  create multiple faces/routes at once (:issue:`5169`)

- Update ``Interest::ForwardingHint`` format (:issue:`5187`)

- Rename the ASF strategy parameter ``n-silent-timeouts`` to ``max-timeouts``

- Allow setting default UDP face MTU in ``nfd.conf`` (:issue:`5138`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Execute ``pcap_activate`` as root for Ethernet faces (:issue:`4647`)

- Update the validation examples in ``nfd.conf.sample`` to accept a certificate name in
  ``KeyLocator`` in addition to a key name (:issue:`5114`)

- Fix and simplify enumeration logic in ``Forwarder::onNewNextHop()``

- Use per-upstream suppression in ASF strategy (:issue:`5140`)

- Use typed name components for (versioned) strategy names (:issue:`5044`)

- Avoid extending the dataset expiration in ``ForwarderStatusManager``

- ``DeadNonceList`` improvements

  * Code cleanup/modernization
  * Prevent duplicate entries (:issue:`5167`)
  * Improve logging (:issue:`5165`)
  * Increase initial and minimum capacity

- Handle error when calling ``remote_endpoint`` on a TCP socket (:issue:`5158`)

- Various build system and documentation extensions and fixes

Removals
^^^^^^^^

- Eliminate ``Forwarder::dispatchToStrategy()`` (use direct call to
  ``StrategyChoice::findEffectiveStrategy(PitEntry)``)

- NCC strategy (legacy)

- Best-route strategy version 1 (legacy)
