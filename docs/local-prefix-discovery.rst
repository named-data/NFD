.. _local-prefix-discovery:

Local hub prefix discovery
==========================

Some applications need to discover the prefix(es) under which they can publish
data / which Interests the local hub will be able to forward down to the
application. In order to discover that, an application can send an Interest for
``/localhop/nfd/rib/routable-prefixes``. The data in response to this Interest
will contain a list of prefixes and should be encoded as follows:

.. code-block:: abnf

    Response = DATA-TYPE TLV-LENGTH
                 Name     ; /localhop/nfd/rib/routable-prefixes/[version]/[segment]
                 MetaInfo ; ContentType == BLOB, FreshnessPeriod == 5000
                 ResponseContent
                 DataSignature

    ResponseContent = 1*Name

``ResponseContent`` should contain at least one ``Name``, which should be routable
towards the face from which the request was received.  The requester may process
the list of returned names and pick whichever it wants to use.

For now, the ``/localhop/nfd/rib/routable-prefixes`` data is served by
:ref:`ndn-autoconfig-server`.
