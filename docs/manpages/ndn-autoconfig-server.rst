.. _ndn-autoconfig-server:

ndn-autoconfig-server
=====================

Usage
-----

::

    ndn-autoconfig-server [-h] [-p <PREFIX>] [-p <PREFIX>] ... <FACEURI>

Description
-----------

``ndn-autoconfig-server`` is a daemon that implements the server part for the stage 1 of
:ref:`NDN hub discovery procedure`.

This daemon essentially waits for Interests for ``/localhop/ndn-autoconf/hub`` and
satisfies them with a Data packet that contains a TLV-encoded FaceUri block.  The value of
this block is the ``Uri`` for the HUB, preferably a UDP tunnel.

``<FACEURI>``
  FaceUri for this NDN hub.

``-p <PREFIX>``
  A local prefix for which the local hub allows end applications to register prefixes
  (See more details in :ref:`local-prefix-discovery`).  One can supply more than one
  prefixes.  All supplied prefixes will be put into the local prefix discovery data
  as described in :ref:`local-prefix-discovery`.  If no prefix is specified,
  auto-config-server will not serve any local prefix discovery data.

``-h``
  Print usage and exit.

``-V``
  Print version number and exit.

Exit status
-----------

0: No error.

1: An unspecified error occurred.

2: Malformed command line, e.g., invalid, missing, or unknown argument.

4: Insufficient privileges.

Examples
--------

::

    ndn-autoconfig-server tcp://spurs.cs.ucla.edu

    ndn-autoconfig-server -p /ndn/edu/ucla tcp://spurs.cs.ucla.edu

See also
--------

:ref:`ndn-autoconfig`
