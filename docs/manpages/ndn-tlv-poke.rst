ndn-tlv-poke
============

Deprecated
----------

ndn-tlv-poke is deprecated and will be removed in a future release of NFD.
Please use ``ndnpoke`` program from ndn-tools repository `<https://github.com/named-data/ndn-tools>`__.

Usage
-----

::

    ndn-tlv-poke [-h] [-f] [-D] [-i identity] [-F] [-x freshness] [-w timeout] name

Description
-----------

``ndn-tlv-poke`` is a simple producer program that reads payload from stdin and publishes it
as a single NDN Data packet.  The Data packet is published either as a response to the
incoming Interest for the given ``name``, or forcefully pushed to the local NDN
forwarder's cache if ``-f`` flag is specified.

The program terminates with return code 0 if Data is sent and with return code 1 when
timeout occurs.

Options
-------

``-h``
  Print usage and exit.

``-f``
  If specified, send Data without waiting for Interest.

``-D``
  If specified, use ``DigestSha256`` signature instead of default ``SignatureSha256WithRsa``.

``-i``
  Use ``identity`` to sign the created Data packet.

``-F``
  Set ``FinalBlockId`` to the last component of specified name.

``-x``
  Set ``FreshnessPeriod`` in milliseconds.

``-w``
  Wait at most ``timeout`` milliseconds for the incoming Interest.  If no Interest arrives
  within the ``timeout``, the Data packet will not be published.


Examples
--------

Create Data packet with content ``hello`` with the name ``ndn:/app/video`` and wait at
most 3 seconds for the incoming Interest for it::

    echo "Hello" | build/bin/ndn-tlv-poke -w 3000 ndn:/app/video
