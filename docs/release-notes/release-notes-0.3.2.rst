NFD version 0.3.2
-----------------

Release date: May 12, 2015

Changes since version 0.3.1:

New features:
^^^^^^^^^^^^^

- **Tables**

  * Change lookup API to allow async implementations  of ContentStore (:issue:`2411`)

- **Management**

  * Perform FIB updates before modifying RIB (:issue:`1941`)

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- **Documentation**

  * Update planned Features: face enhancements (:issue:`2617`)

  * Updated NFD's developer guide to reflect new changes and adding a new section on
    NFD/ndn-cxx unit testing

- **Face**

  * Refactor channel acceptors to avoid use of shared pointers (:issue:`2613`)

  * Refactor code to avoid deprecated `Block::fromBuffer` overloads (:issue:`2553`)

  * Refactor code to use move semantics for sockets where possible (:issue:`2613`)

  * Switch to non-throwing version of websocketpp APIs (:issue:`2630`)

- **Forwarding**

  * Extend measurements lifetime in AccessStrategy (:issue:`2452`)

- **Management**

  * Stop removed namespace from inheriting ancestor route (:issue:`2757`)

  * Fix TestFibUpdates/EraseFace on Ubuntu 14.04 32-bit (:issue:`2697`)

- **Tools**

  * Fix hanging of `nfdc` on wrong input (:issue:`2690`)

- **Build**

  * Make build scripts compatible with Python 3 (:issue:`2625`)

  * Get rid of the last use of ndn::dns in NFD (:issue:`2422`)

  * Update waf build system to version 1.8.9 (:issue:`2209`, :issue:`2657`, :issue:`2792`)

  * Tweak default pkg-config search paths (:issue:`2711`)

  * Use C version of snprintf (:issue:`2299`)

  * Emulate `std::to_string` when it is missing (:issue:`2299`)

  * Fix several "defined but not used" warnings with gcc-5 (:issue:`2767`)

  * Disable precompiled headers on OS X with clang < 6.1.0 (:issue:`2804`)
