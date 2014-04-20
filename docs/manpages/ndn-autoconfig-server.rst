.. _ndn-autoconfig-server:

ndn-autoconfig-server
=====================

Usage
-----

::

    ndn-autoconfig-server [-h] FaceUri


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

Examples
--------

::

    ndn-autoconfig-server tcp://spurs.cs.ucla.edu


See also
--------

:ref:`ndn-autoconfig`
