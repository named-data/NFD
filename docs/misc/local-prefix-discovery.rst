.. _local-prefix-discovery:

Discover local hub prefix
=========================

Some applications need to discover prefix(es) under which they can publish data
/ which Interests local hub will be able to forward down to the application.
In order to discover that, applications need to send an interest for
``/localhop/nfd/rib/routable-prefixes`` prefix. Response data to the
interest contains a list of prefixes and should be encoded as:

::

    Response ::= DATA-TYPE TLV-LENGTH
                 Name (= /localhop/nfd/rib/routable-prefixes/[version]/[segment])
                 MetaInfo (= ResponseMetaInfo)
                 Content (= ResponseContent)
                 Signature

    ResponseMetaInfo ::= META-INFO-TYPE TLV-LENGTH
                         ContentType (= DATA)
                         FreshnessPeriod (= 5000)

    ResponseContent ::= Name+

.. note::
    ResponseContent should contain at least one Name, which should be routable
    towards the face from which the request has been received.  The requester may
    process list of the returned names and pick whichever it wants to use.

For now, the ``/localhop/nfd/rib/routable-prefixes`` data is served by
:ref:`ndn-autoconfig-server`.
