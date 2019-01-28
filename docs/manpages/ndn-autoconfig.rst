.. _ndn-autoconfig:

ndn-autoconfig
==============

Usage
-----

::

    ndn-autoconfig [options]

Description
-----------

Client tool to run :ref:`NDN hub discovery procedure`.

Options
-------

``-d`` or ``--daemon``
  Run ndn-autoconfig in daemon mode. In this mode, the auto-discovery procedure is re-run
  hourly or when a network change event is detected.

  NOTE: if connection to NFD fails, the daemon will be terminated.

``-c [FILE]`` or ``--config=[FILE]``
  Use the specified configuration file. If `enabled = true` is not specified in the
  configuration file, no actions will be performed.

``--ndn-fch-url=[URL]``
  Use the specified URL to find the closest hub (NDN-FCH protocol).  If not specified,
  ``http://ndn-fch.named-data.net/`` will be used.  Only ``http://`` URLs are supported.

``-h`` or ``--help``
  Print help message and exit.

``-V`` or ``--version``
  Show version information and exit.

.. _NDN hub discovery procedure:

NDN hub discovery procedure
---------------------------

When an end host starts up, or detects a change in its network environment, it MAY use
this procedure to discover a NDN router, in order to gain connectivity to
`the NDN research testbed <https://named-data.net/ndn-testbed/>`__.
This procedure can discover either an NDN router in the local network, or a NDN testbed
gateway router (commonly known as a "hub").

Overview
^^^^^^^^

This procedure contains four methods to discover a NDN router:

1.  Look for a local NDN router by multicast.
    This is useful in a home or small office network.

2.  Look for a local NDN router by DNS query with default suffix.
    This allows network administrator to configure a NDN router in a large enterprise network.

3.  Find closest hub by sending an HTTP request to NDN-FCH server.

4.  Connect to the home hub according to user certificate.
    This ensures connectivity from anywhere.

After connecting, two prefixes will be registered toward the router:

- ``/`` --- this allows application communication
- ``/localhop/nfd`` --- this informs NFD-RIB that there is connectivity to a router

Stage 1: multicast discovery
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Request
+++++++

The end host sends Interest ``/localhop/ndn-autoconf/hub`` over a multicast face.

Response
++++++++

A producer app on the router answers this Interest with a Data packet that contains a
``Uri`` TLV element.  The value of this element is the FaceUri for the router, such as
a UDP tunnel.

Stage 2: DNS query with default suffix
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Request
+++++++

The end host sends a DNS query that is equivalent to this command::

    dig +search +short +cmd +tries=2 +ndots=10 _ndn._udp srv

Response
++++++++

The DNS server should answer with an SRV record that contains the hostname and UDP port
number of a nearby NDN router.

Stage 3: HTTP Request to NDN-FCH server
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This stage uses a simple HTTP-based API.  For more information about NDN-FCH server, refer
to the `NDN-FCH README file <https://github.com/named-data/ndn-fch>`__.

Request
+++++++

HTTP/1.0 request for the NDN-FCH server URI (`<http://ndn-fch.named-data.net/>`__ by default)

Response
++++++++

The HTTP response is expected to be a hostname or an IP address of the closest hub,
inferred using IP-geo approximation service.

Stage 4: find home router
^^^^^^^^^^^^^^^^^^^^^^^^^

This stage assumes that user has configured default certificate using
`<https://ndncert.named-data.net/>`__ as described in `Certification Architecture
<https://redmine.named-data.net/attachments/download/23/CertificationArchitecture.pptx>`__.

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
number of the home hub of this user's site.

Client procedure
----------------

Stage 1
^^^^^^^

Send a multicast discovery Interest.
If this Interest is answered, connect to the router and terminate auto-discovery.

Stage 2
^^^^^^^

Send a DNS query with default suffix.
If this query is answered, connect to the router and terminate auto-discovery.

Stage 3
^^^^^^^

Send HTTP request to NDN-FCH server.
If request succeeds, attempt to connect to the discovered hub and terminate
auto-discovery.

Stage 4
^^^^^^^

Load default user identity, and convert it to DNS format.
If either fails, the auto-discovery fails.

Send a DNS query to find home hub.
If this query is answered, connect to the home hub and terminate auto-discovery.
Otherwise, the auto-discovery fails.

Exit status
-----------

0: No error.

1: An unspecified error occurred.

2: Malformed command line, e.g., invalid, missing, or unknown argument.

4: Insufficient privileges.

See also
--------

:ref:`ndn-autoconfig-server`, :doc:`ndn-autoconfig.conf`
