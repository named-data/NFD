.. _ndn-autoconfig:

ndn-autoconfig
==============

Usage
-----

::

    ndn-autoconfig

Description
-----------

Client tool to run :ref:`NDN hub discovery procedure`.

.. _NDN hub discovery procedure:

NDN hub discovery procedure
---------------------------

When an end host starts up, or detects a change in its network environment, it MAY use
this procedure to discover a local or home NDN router, in order to gain connectivity to
`the NDN research testbed <http://named-data.net/ndn-testbed/>`_.

Overview
^^^^^^^^

This procedure contains three methods to discover a NDN router:

1.  Look for a local NDN router by multicast.
    This is useful in a home or small office network.

2.  Look for a local NDN router by DNS query with default suffix.
    This allows network administrator to configure a NDN router in a large enterprise network.

3.  Connect to the home NDN router according to user certificate.
    This ensures connectivity from anywhere.

After connecting to an NDN router, two prefixes will be automatically registered:

- /ndn
- /localhop/nfd --- this to inform RIB manager that there is connectivity to the hub

Stage 1: multicast discovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Request
+++++++

The end host sends an Interest over a multicast face.

Interest Name is ``/localhop/ndn-autoconf/hub``.

Response
++++++++

A producer app on the HUB answer this Interest with a Data packet that contains a
TLV-encoded `Uri` block.  The value of this block is the URI for the HUB, preferrably a
UDP tunnel.

Stage 2: DNS query with default suffix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Request
+++++++

The end host sends a DNS query that is equivalent to this command::

    dig +search +short +cmd +tries=2 +ndots=10 _ndn._udp srv

Response
++++++++

The DNS server should answer with an SRV record that contains the hostname and UDP port
number of the NDN router.

Stage 3: find home router
^^^^^^^^^^^^^^^^^^^^^^^^^

This stage assumes that user has configured default certificate using
`<http://ndncert.named-data.net/>`_ as described in `Certification Architecture
<http://redmine.named-data.net/attachments/download/23/CertificationArchitecture.pptx>`_.

Request
+++++++

The end host loads the default user identity (eg. ``/ndn/edu/ucla/cs/afanasev``), and
converts it to DNS format.

The end host sends a DNS query for an SRV record of name ``_ndn._udp.`` + user identity in
DNS format + ``_homehub._autoconf.named-data.net``. For example::

    _ndn._udp.afanasev.cs.ucla.edu.ndn._homehub._autoconf.named-data.net

Response
++++++++

The DNS server should answer with an SRV record that contains the hostname and UDP port
number of the home NDN router of this user's site.

Client procedure
----------------

Stage 1
^^^^^^^

Send a multicast discovery Interest.

If this Interest is answered, connect to the HUB and terminate auto-discovery.

Stage 2
^^^^^^^

Send a DNS query with default suffix.

If this query is answered, connect to the HUB and terminate auto-discovery.

Stage 3
^^^^^^^

* Load default user identity, and convert it to DNS format; if either fails, the
  auto-discovery fails.

* Send a DNS query to find home HUB.
  If this query is answered, connect to the home HUB and terminate auto-discovery.
  Otherwise, the auto-discovery fails.


See also
--------

:ref:`ndn-autoconfig-server`
