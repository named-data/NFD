.. _ndn-autoconfig-server:

ndn-autoconfig-server
=====================

Usage
-----

::

    ndn-autoconfig-server [-h] [-p prefix] [-p prefix] ... FaceUri


Description
-----------

``ndn-autoconfig-server`` is a daemon that implements server part for the stage 1 of
:ref:`NDN hub discovery procedure`.

This daemon essentially waits for Interests for ``/localhop/ndn-autoconf/hub`` and
satisfies them with a Data packet that contains TLV-encoded FaceUri block.  The value of
this block is the ``Uri`` for the HUB, preferrably a UDP tunnel.

``-h``
  print usage and exit.

``FaceUri``
  FaceUri for this NDN hub.

``-p prefix``
  A local prefix for which the local hub allow end applications to register prefix
  (See more details in :ref:`local-prefix-discovery`).  One can supply more than one
  prefixes.  All supplied prefixes will be put into the local prefix discovery data
  as described in :ref:`local-prefix-discovery`.  If no prefix is specified,
  auto-config-server will not serve any local prefix discovery data.

Examples
--------

::

    ndn-autoconfig-server tcp://spurs.cs.ucla.edu

    ndn-autoconfig-server -p /ndn/edu/ucla tcp://spurs.cs.ucla.edu


See also
--------

:ref:`ndn-autoconfig`
