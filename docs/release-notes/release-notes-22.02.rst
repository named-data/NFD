NFD version 22.02
-----------------

Release date: February 17, 2022

Note that starting with this release, NFD switched to a date-based versioning scheme:
``YEAR.MONTH[.PATCH]`` (``YY.0M[.MICRO]`` in `CalVer <https://calver.org/>`__ syntax).

Notable changes and new features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- Dispatch pending interests when a new next hop is created in :nfd:`MulticastStrategy`
  (:issue:`4931`)

- Remove Nacks from :nfd:`MulticastStrategy` (:issue:`5146`)

- Rename the ASF strategy parameter ``n-silent-timeouts`` to ``max-timeouts``

- Reorder function parameters in :nfd:`Strategy` class to make the API more uniform
  (:issue:`5173`)

- Add default ``HopLimit`` to Interests when missing (:issue:`5171`)

- Update Interest ``ForwardingHint`` format (:issue:`5187`)

- Allow setting default UDP face MTU in ``nfd.conf`` (:issue:`5138`)

- Allow batch command processing by ``nfdc`` to accommodate the case when ``nfdc`` is
  used to create multiple faces/routes at once (:issue:`5169`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Execute ``pcap_activate`` as root for Ethernet faces (:issue:`4647`)

- Update the validation examples in ``nfd.conf.sample`` to accept a certificate name in
  ``KeyLocator`` in addition to a key name (:issue:`5114`)

- Fix and simplify enumeration logic in ``Forwarder::onNewNextHop()``

- Use per-upstream retx suppression in ASF strategy (:issue:`5140`)

- Use typed name components for (versioned) strategy names (:issue:`5044`)

- :nfd:`DeadNonceList` improvements

  * Code cleanup/modernization
  * Prevent duplicate entries (:issue:`5167`)
  * Increase initial and minimum capacity
  * Improve logging (:issue:`5165`)

- Use the default dataset expiration in :nfd:`ForwarderStatusManager`

- Handle error when calling ``remote_endpoint`` on a TCP socket (:issue:`5158`)

- Various build system and documentation extensions and fixes

Removals
^^^^^^^^

- Best-route strategy version 1 (legacy)

- NCC strategy (legacy)
