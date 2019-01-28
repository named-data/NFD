.. _v0.6.3:

NFD version 0.6.3
-----------------

Release date: September 18, 2018

The build requirements have been upgraded to gcc >= 5.3 or clang >= 3.6, boost >= 1.58,
openssl >= 1.0.2. This effectively drops support for all versions of Ubuntu older than 16.04
that use distribution-provided compilers and packages.

The compilation now uses the C++14 standard.

New features
^^^^^^^^^^^^

- Allow MTU of datagram faces to be overridden (:issue:`4005`, :issue:`4789`)

- Implement ``nfdc cs erase`` command (:issue:`4318`)

- Initial framework to realize self-learning feature

  * RIB code refactoring (:issue:`4650`, :issue:`4723`, :issue:`4683`)

  * Add facility to execute functions on the RIB and main threads (:issue:`4279`, :issue:`4683`)

  * Incorporate ``PrefixAnnouncement`` into ``Route`` and ``RibEntry`` (:issue:`4650`)

- Add official support for CentOS 7 (:issue:`4610`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Make LRU the default CS replacement policy (:issue:`4728`)

- Refactor logging to use ndn-cxx logging framework (:issue:`4580`)

- Directly use ``asio::ip::address::from_string`` instead of ndn-cxx provided helpers
  (Boost.Asio >= 1.58 includes necessary fixes)

- Avoid use of deprecated {get,set}FinalBlockId

- Improve and simplify code with modern C++ features

- Improve test cases

- Improve documentation
