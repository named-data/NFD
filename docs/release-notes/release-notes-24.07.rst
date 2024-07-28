NFD version 24.07
-----------------

*Release date: July 28, 2024*

Important changes and new features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- The build dependencies have been increased as follows:

  - GCC >= 9.3 or Clang >= 7.0 are strongly *recommended* on Linux; GCC 8.x is also known
    to work but is not officially supported
  - Xcode 13 or later is *recommended* on macOS; older versions may still work but are not
    officially supported
  - Boost >= 1.71.0 is *required* on all platforms

- Ubuntu 24.04 (Noble), Debian 12 (Bookworm), and macOS 14 (Sonoma) are now officially supported

- Added an official Dockerfile to the repository. From this Dockerfile, the following images are
  currently published to the GitHub container registry:

  - `named-data/nfd <https://github.com/named-data/NFD/pkgs/container/nfd>`__
  - `named-data/nfd-autoreg <https://github.com/named-data/NFD/pkgs/container/nfd-autoreg>`__
  - `named-data/nfd-status-http-server <https://github.com/named-data/NFD/pkgs/container/nfd-status-http-server>`__

  All images are available for *linux/amd64* and *linux/arm64* platforms.

- The default Unix socket path changed to ``/run/nfd/nfd.sock`` on Linux and to
  ``/var/run/nfd/nfd.sock`` on all other platforms (:issue:`5304`)

- Introduced a new strategy trigger :nfd:`onInterestLoop <Strategy::onInterestLoop>` that is
  invoked when a duplicate Interest is received. The default behavior (sending a Nack packet)
  remains unchanged except for the Multicast strategy, which will now suppress the Nack
  transmission in this case (:issue:`5278`)

- Multiple updates to the ASF forwarding strategy to more closely adhere to the behavior
  described in the published technical report (:issue:`5310`)

- The ASF strategy gained support for Nack propagation (:issue:`5311`)

- The default port number of ``nfd-status-http-server`` changed to 6380

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Several stability improvements in the face system, especially around error handling in
  multicast faces and :nfd:`UnixStreamChannel` (:issue:`5030`, :issue:`5297`)

- Refactor and improve logging in forwarding core and strategies (:issue:`5262`, :issue:`5267`)

- Fix missing increment of ``nOutNacks`` counter when sending a Nack from ``onInterestLoop``

- Optimize the removal of PIT in-records

- Move RIB unit tests into ``unit-tests-daemon`` and eliminate the ``unit-tests-rib`` binary

- Fix building the documentation with Python 3.12 (:issue:`5298`)

- Reduce amount of debugging information produced in compiled binaries by default (:issue:`5279`)

- Update waf build system to version 2.0.27

- Various code cleanups, modernizations, and documentation improvements
