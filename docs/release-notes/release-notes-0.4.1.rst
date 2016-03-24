NFD version 0.4.1
-----------------

Release date: March 25, 2016

Changes since version 0.4.0:

Updates and bug fixes:
^^^^^^^^^^^^^^^^^^^^^^

- Fix retrieval of data using full names, i.e., names with the implicit digest (:issue:`3363`)

- Fix PIT memory leak from duplicate Interests detected using DNL table (:issue:`3484`)

- Fix crashes caused by some patterns of prefix registrations (:issue:`3404`, :issue:`3362`)

- Allow setting CS capacity limit to zero (:issue:`3503`)

- Improve Face (:issue:`3376`), UdpFactory (:issue:`3375`), UdpChannel (:issue:`3371`),
  TcpChannel (:issue:`3370`), WebSocketChannel (:issue:`3373`), WebSocketTransport
  (:issue:`3374`) test suites

- Remove assumption from the management test suites that data packets are always published
  in NFD's content store (:issue:`2182`)

Deleted:
^^^^^^^^

- ``ndn-tlv-peek`` and ``ndn-tlv-poke`` command-line tools: use ``ndnpeek`` and ``ndnpoke``
  programs from `NDN Essential Tools repository <https://github.com/named-data/ndn-tools>`__
  (:issue:`2819`)
