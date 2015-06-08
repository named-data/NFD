ndn-tlv-peek
============

Deprecated
----------

ndn-tlv-peek is deprecated and will be removed in a future release of NFD.
Please use ``ndnpeek`` program from ndn-tools repository `<https://github.com/named-data/ndn-tools>`__.

Usage
-----

::

    ndn-tlv-peek [-h] [-f] [-r] [-m min] [-M max] [-l lifetime] [-p] [-w timeout] name

Description
-----------

``ndn-tlv-peek`` is a simple consumer program that sends one Interest and expects one Data
packet in response.  The full Data packet (in TLV format) is written to stdout.  The
program terminates with return code 0 if Data arrives, and with return code 1 when timeout
occurs.

``name`` is interpreted as the Interest name.

Options
-------

``-h``
  Print help and exit

``-f``
  If specified, set ``MustBeFresh`` selector in the Interest packet.

``-r``
  Set ``ChildSelector=1`` (the rightmost child) selector.

``-m``
  Set ``min`` as the ``MinSuffixComponents`` selector.

``-M``
  Set ``max`` as the ``MaxSuffixComponents`` selector.

``-l``
  Set ``lifetime`` (in milliseconds) as the ``InterestLifetime``.

``-p``
  If specified, print the received payload only, not the full packet.

``-w``
  Timeout after ``timeout`` milliseconds.


Examples
--------

Send Interest for ``ndn:/app1/video`` and print the received payload only

::

    ndn-tlv-peek -p ndn:/app1/video
