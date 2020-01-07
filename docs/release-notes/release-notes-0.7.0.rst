NFD version 0.7.0
-----------------

Release date: January 13, 2020

New features
^^^^^^^^^^^^

- Support only `NDN packet format version 0.3 <https://named-data.net/doc/NDN-packet-spec/0.3/>`__
  (:issue:`4805`, :issue:`4581`, :issue:`4913`)

- Initial support for ``PitToken`` (:issue:`4532`)

- Random Forwarding Strategy that chooses a random outgoing face, excluding the incoming
  face of an Interest packet (:issue:`4992`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Avoid ``cs::Entry`` construction during Content Store lookup (:issue:`4914`)

- Make use of ndn-cxx's ``RttEstimator`` in ``LpReliability``, ``AsfStrategy``, and
  ``AccessStrategy`` (:issue:`4887`)

- Increase pcap buffer size in Ethernet faces (:issue:`2441`)

- Make congestion marking less aggressive (:issue:`5003`). The updated implementation is
  closer to the CoDel algorithm (RFC 8289)

- Pull ``FaceTable`` construction out of ``Forwarder`` class (:issue:`4922`, :issue:`4973`)

- Enable direct certificate fetch in ``RibManager`` (:issue:`2237`)

- Refuse to start NFD if both ``localhop_security`` and ``auto_prefix_propagate`` are
  enabled in the configuration file (:issue:`4989`)

- Fix crash in self-learning strategy when the out-record of a PIT
  entry does not exist (:issue:`5022`)

- Use a separate validator and a separate configuration section in ``nfd.conf`` for prefix
  announcements (:issue:`5031`).  The default configuration will accept any prefix
  announcement.

- Documentation improvements
