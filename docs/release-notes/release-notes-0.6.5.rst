NFD version 0.6.5
-----------------

Release date: February 4, 2019

New features
^^^^^^^^^^^^

- Self-learning forwarding strategy (:issue:`4279`)

- Support IPv6-only WebSocket channels (:issue:`4710`)

- Add satisfied and unsatisfied counters to forwarder status management and tools (:issue:`4720`)

- Report face MTU in faces/list and faces/query datasets (:issue:`4763`)

Improvements and bug fixes
^^^^^^^^^^^^^^^^^^^^^^^^^^

- Stop using the global scheduler in RIB (:issue:`4528`)

- Reimplement auto prefix propagation on top of the readvertise framework (:issue:`3819`)

- Don't set ``SO_BINDTODEVICE`` on Android (:issue:`4761`)

- Set ``EthernetTransport`` UP/DOWN based on netif state (:issue:`3352`)

- Update WebSocket++ to 0.8.1

- Add ``EndpointId`` field in ``NextHop`` record (:issue:`4284`)

- Seed the global PRNG with more entropy

- Use ndn-cxx ``scheduler::ScopedEventId`` (:issue:`4698`)

- Improvements with systemd integration (:issue:`2815`)

Removed
^^^^^^^

- Remove obsolete upstart files

- Delete deprecated ``ClientControlStrategy`` (:issue:`3783`)
