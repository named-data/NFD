.. _local-prefix-discovery:

Local hub prefix discovery
==========================

Some applications need to discover the prefix(es) under which they can publish data
/ which Interests the local hub will be able to forward down to the application.
In order to discover that, applications need to send an Interest for
``/localhop/nfd/rib/routable-prefixes`` prefix. Response data to the
Interest contains a list of prefixes and should be encoded as follows:

::

    Response ::= DATA-TYPE TLV-LENGTH
                   Name (= /localhop/nfd/rib/routable-prefixes/[version]/[segment])
                   MetaInfo (= ResponseMetaInfo)
                   Content (= ResponseContent)
                   Signature

    ResponseMetaInfo ::= META-INFO-TYPE TLV-LENGTH
                           ContentType (= BLOB)
                           FreshnessPeriod (= 5000)

    ResponseContent ::= Name+

.. note::
    ResponseContent should contain at least one ``Name``, which should be routable
    towards the face from which the request has been received.  The requester may
    process the list of returned names and pick whichever it wants to use.

For now, the ``/localhop/nfd/rib/routable-prefixes`` data is served by
:ref:`ndn-autoconfig-server`.
