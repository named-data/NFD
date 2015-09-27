NFD version 0.2.0
-----------------

Release date: August 25, 2014

Changes since version 0.1.0:

- **Documentation**

  + `"NFD Developer's Guide" by NFD authors
    <http://named-data.net/wp-content/uploads/2014/07/NFD-developer-guide.pdf>`_ that
    explains NFD's internals including the overall design, major modules, their
    implementation, and their interactions

  + New detailed instructions on how to enable auto-start of NFD using OSX ``launchd``
    and Ubuntu's ``upstart`` (see `contrib/ folder
    <https://github.com/named-data/NFD/tree/master/contrib>`_)

- **Core**

  + Add support for temporary privilege drop and elevation (:issue:`1370`)

  + Add support to reinitialize multicast Faces and (partially) reload config file
    (:issue:`1584`)

  + Randomization routines are now uniform across all NFD modules (:issue:`1369`)

  + Enable use of new NDN naming conventions (:issue:`1837` and :issue:`1838`)

- **Faces**

  + `WebSocket <http://tools.ietf.org/html/rfc6455>`_ Face support (:issue:`1468`)

  + Fix Ethernet Face support on Linux with ``libpcap`` version >=1.5.0 (:issue:`1511`)

  + Fix to recognize IPv4-mapped IPv6 addresses in ``FaceUri`` (:issue:`1635`)

  + Fix to avoid multiple onFail events (:issue:`1497`)

  + Fix broken support of multicast UDP Faces on OSX (:issue:`1668`)

  + On Linux, path MTU discovery on unicast UDPv4 faces is now disabled (:issue:`1651`)

  + Added link layer byte counts in FaceCounters (:issue:`1729`)

  + Face IDs 0-255 are now reserved for internal NFD use (:issue:`1620`)

  + Serialized StreamFace::send(Interest|Data) operations using queue (:issue:`1777`)

- **Forwarding**

  + Outgoing Interest pipeline now allows strategies to request a fresh ``Nonce`` (e.g., when
    the strategy needs to re-express the Interest) (:issue:`1596`)

  + Fix in the incoming Data pipeline to avoid sending packets to the incoming Face
    (:issue:`1556`)

  + New ``RttEstimator`` class that implements the Mean-Deviation RTT estimator to be used in
    forwarding strategies

  + Fix memory leak caused by not removing PIT entry when Interest matches CS (:issue:`1882`)

  + Fix spurious assertion in NCC strategy (:issue:`1853`)

- **Tables**

  + Fix in ContentStore to properly adjust internal structure when ``Cs::setLimit`` is called
    (:issue:`1646`)

  + New option in configuration file to set an upper bound on ContentStore size (:issue:`1623`)

  + Fix to prevent infinite lifetime of Measurement entries (:issue:`1665`)

  + Introducing capacity limit in PIT NonceList (:issue:`1770`)

  + Fix memory leak in NameTree (:issue:`1803`)

  + Fix segfault during Fib::removeNextHopFromAllEntries (:issue:`1816`)

- **Management**

  + RibManager now fully support ``CHILD_INHERIT`` and ``CAPTURE`` flags (:issue:`1325`)

  + Fix in ``FaceManager`` to respond with canonical form of Face URI for Face creation command
    (:issue:`1619`)

  + Fix to prevent creation of duplicate TCP/UDP Faces due to async calls (:issue:`1680`)

  + Fix to properly handle optional ExpirationPeriod in RibRegister command (:issue:`1772`)

  + Added functionality of publishing RIB status (RIB dataset) by RibManager (:issue:`1662`)

  + Fix issue of not properly canceling route expiration during processing of ``unregister``
    command (:issue:`1902`)

  + Enable periodic clean up of route entries that refer to non-existing faces (:issue:`1875`)

- **Tools**

  + Extended functionality of ``nfd-status``

     * ``-x`` to output in XML format, see :ref:`nfd-status xml schema`
     * ``-c`` to retrieve channel status information (enabled by default)
     * ``-s`` to retrieve configured strategy choice for NDN namespaces (enabled by default)
     * Face status now includes reporting of Face flags (``local`` and ``on-demand``)
     * On-demand UDP Faces now report remaining lifetime (``expirationPeriod``)
     * ``-r`` to retrieve RIB information

  + Improved ``nfd-status-http-server``

     * HTTP server now presents status as XSL-formatted XML page
     * XML dataset and formatted page now include certificate name of the corresponding NFD
       (:issue:`1807`)

  + Several fixes in ``ndn-autoconfig`` tool (:issue:`1595`)

  + Extended options in ``nfdc``:

    * ``-e`` to set expiration time for registered routes
    * ``-o`` to specify origin for registration and unregistration commands

  + Enable ``all-faces-prefix'' option in ``nfd-autoreg`` to register prefix for all face
    (on-demand and non-on-demand) (:issue:`1861`)

  + Enable processing auto-registration in ``nfd-autoreg`` for faces that existed prior to
    start of the tool (:issue:`1863`)

- **Build**

  + Enable support of precompiled headers for clang and gcc to speed up compilation

- Other small fixes and extensions
