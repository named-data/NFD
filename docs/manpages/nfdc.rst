nfdc
====

Usage
-----

::

    nfdc [-h] COMMAND [<command options>]


Description
-----------

``nfdc`` is a tool to manipulate routing information base (RIB), forwarding information
base (FIB), and StrategyChoices table (i.e., which strategy should be used by which
namespaces).

Options
-------

``-h``
  Print usage information.

``COMMAND``

  ``register``
    Register a new or update existing routing entry in Routing Information Base (RIB).

    ``register [-I] [-C] [-c <cost>] [-e expiration time] [-o origin] <prefix> <faceId | faceUri>``

      ``-I``
        Unset CHILD_INHERIT flag from the routing entry.

      ``-C``
        Set CAPTURE flag in the routing entry.

      ``-c <cost>``
        Cost for the RIB entry (default is 0).

      ``-e <expiration time>``
        Expiration time of the RIB entry in milliseconds. If not specified, the entry remains in FIB
        for the lifetime of the associated face.

      ``-o <origin>``
        Origin of the registration request (default is 255).
        0 for Local producer applications, 128 for NLSR, 255 for static routes.

      ``prefix``
        A prefix of an existing or to be created RIB entry, for which routing entry is
        requested to be added or updated.

      ``faceId``
        An existing NFD Face ID number, which can be obtained, for example, using
        ``nfd-status`` command.

      ``faceUri``
        URI of the existing or to be created Face.

  ``unregister``
    Unregister an existing routing entry from Routing Information Base (RIB).

    ``unregister [-o origin] <prefix> <faceId>``

      ``-o <origin>``
        Origin of the unregistration request (default is 255).

      ``prefix``
        A prefix of an existing RIB entry, from which routing entry is requested to be
        removed.

      ``faceId``
        An existing NFD Face ID number, which can be obtained, for example, using
        ``nfd-status`` command.

  ``create``
    Create a UDP unicast or TCP Face

    ``create <faceUri>``

      ``faceUri``
        UDP unicast or TCP Face URI::

            UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]
            TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>]

  ``destroy``
    Create an existing UDP unicast or TCP Face.

    ``destroy <faceId | faceUri>``

      ``faceId``
        An existing NFD Face ID number, which can be obtained, for example, using
        ``nfd-status`` command.

      ``faceUri``
        UDP unicast or TCP Face URI::

            UDP unicast:    udp[4|6]://<remote-IP-or-host>[:<remote-port>]
            TCP:            tcp[4|6]://<remote-IP-or-host>[:<remote-port>]

  ``set-strategy``
    Select strategy to be used for the specified namespace

    ``set-strategy <namespace> <strategy-name>``

      ``namespace``
        Namespace that will use the specified strategy.

        Note that more specific namespace(s) can use different strategy or strategies.
        For example, if namespace ``/A/B/C`` was using strategy
        ``ndn:/localhost/nfd/strategy/best-route`` before running ``set-strategy`` on
        ``/A`` namespace, it will continue using the same strategy no matter which
        namespace was specified for ``/A``.

      ``strategy-name``
        Name of one of the available strategies.

        Currently, NFD supports the following strategies::

            ndn:/localhost/nfd/strategy/best-route
            ndn:/localhost/nfd/strategy/multicast
            ndn:/localhost/nfd/strategy/client-control
            ndn:/localhost/nfd/strategy/ncc
            ndn:/localhost/nfd/strategy/access

  ``unset-strategy``
    Unset the strategy for a given ``namespace``.

    Effectively, this command select parent's namespace strategy to be used for the
    specified ``namespace``.

    ``unset-strategy <namespace>``

      ``namespace``
        Namespace from which namespace customization should be removed.

  ``add-nexthop``
    Directly add nexthop entry info NFD's Forwarding Information Base (FIB).  This command
    is intended only for debugging purposes.  Normally, prefix-nexhop association should
    be registered in Routing Information Base using ``register`` command.

    ``add-nexthop [-c <cost>] <prefix> <faceId | faceUri>``

      ``-c <cost>``
        Cost for the nexthop entry to be inserted (default is 0).

      ``prefix``
        A prefix of an existing or to be created FIB entry, to which nexthop
        entry is requested to be added.

      ``faceId``
        An existing NFD Face ID number, which can be obtained, for example, using
        ``nfd-status`` command

      ``faceUri``
        URI of the existing or to be created Face.

  ``remove-nexthop``
    Directly remove nexthop entry from NFD'S FIB.  This command
    is intended only for debugging purposes.  Normally, prefix-nexhop association should
    be unregistered from Routing Information Base using ``unregister`` command.

    ``remove-nexthop <prefix> <faceId>``

      ``prefix``
        A prefix of an existing FIB entry, from which nexthop entry is requested to be removed.

      ``faceId``
        An existing NFD Face ID number, which can be obtained, for example, using
        ``nfd-status`` command.

        Note that when ``faceId`` is the last Face associated with ``prefix`` FIB entry,
        the whole FIB entry will be removed.



Examples
--------

Add a namespace to a face uri:

::

    nfdc register ndn:/app1/video udp://192.168.1.2

Set strategy to a name:

::

    nfdc set-strategy ndn:/app1/video ndn:/localhost/nfd/strategy/broadcast
