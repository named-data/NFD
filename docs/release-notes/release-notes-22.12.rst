NFD version 22.12
-----------------

Release date: December 31, 2022

Important changes and new features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

- NFD now uses the C++17 standard to build

- The minimum build requirements have been increased as follows:

  - Either GCC >= 7.4.0 or Clang >= 6.0 is required on Linux
  - On macOS, Xcode 11.3 or later is recommended; older versions may still work but are not
    officially supported
  - Boost >= 1.65.1 and ndn-cxx >= 0.8.1 are required on all platforms
  - Sphinx 4.0 or later is required to build the documentation

- CentOS Stream 9 is now officially supported; CentOS 8 has been dropped (:issue:`5181`)

- macOS 12 (Monterey) and 13 (Ventura) running on arm64 are now officially supported
  (:issue:`5135`)

- The ASF, BestRoute, and Multicast strategies gained support for fine-grained configuration
  of the retransmission suppression parameters (:issue:`4924`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Prevent Ethernet faces from hanging NFD when a network interface goes down

- Refactor the ``EndpointId`` implementation using ``std::variant`` (:issue:`5041`)

- Better support for the new signed Interest format in management

- Extend and optimize the use of precompiled headers (:issue:`5212`)

- Stop using the ``gold`` linker on Linux; prefer instead linking with ``lld`` if installed

- Update waf build system to version 2.0.24

- Various documentation improvements
