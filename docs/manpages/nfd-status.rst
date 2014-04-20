nfd-status
==========

Usage
-----

::

    nfd-status [options]

Description
-----------

``nfd-status`` is a tool to retrieve and print out NFD version and status information.

Options:
--------

``-h``
  Print usage information.

``-v``
  Retrieve version information.

``-f``
  Retrieve face status information.

``-b``
  Retrieve FIB information.

If no options are provided, all information is retrieved.

Examples
--------

Get all status information from NFD::

    $ nfd-status
    General NFD status:
                   version=1000
                 startTime=20140427T230936.006000
               currentTime=20140427T231055.924000
                    uptime=79 seconds
          nNameTreeEntries=12
               nFibEntries=4
               nPitEntries=2
      nMeasurementsEntries=0
                nCsEntries=24
              nInInterests=20
             nOutInterests=19
                  nInDatas=24
                 nOutDatas=19
    Faces:
      faceid=1 remote=internal:// local=internal:// counters={in={0i 24d} out={20i 0d}}
      faceid=2 remote=udp4://224.0.23.170:56363 local=udp4://192.168.0.106:56363 counters={in={0i 0d} out={0i 0d}}
      faceid=3 remote=udp4://224.0.23.170:56363 local=udp4://127.0.0.1:56363 counters={in={0i 0d} out={0i 0d}}
      faceid=5 remote=fd://16 local=unix:///private/tmp/nfd.sock counters={in={12i 1d} out={1i 11d}}
      faceid=8 remote=tcp4://131.179.196.46:6363 local=tcp4://192.168.0.106:56118 counters={in={0i 0d} out={0i 0d}}
      faceid=9 remote=fd://17 local=unix:///private/tmp/nfd.sock counters={in={2i 0d} out={0i 1d}}
    FIB:
      /localhost/nfd nexthops={faceid=1 (cost=0)}
      /localhop/nfd/rib nexthops={faceid=5 (cost=0)}
      /ndn nexthops={faceid=8 (cost=0)}
      /localhost/nfd/rib nexthops={faceid=5 (cost=0)}
