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

``-c``
  Retrieve channel status information.

``-f``
  Retrieve face status information.

``-b``
  Retrieve FIB information.

``-s``
  Retrieve configured strategy choice for NDN namespaces.

``-x``
  Output NFD status information in XML format.

``-V``
  Show version information of nfd-status and exit.

If no options are provided, all information is retrieved.

If -x is provided, other options(-v, -c, etc.) are ignored, and all information is printed in XML format.

Examples
--------

Get all status information from NFD::

    $ nfd-status
    General NFD status:
                   version=1000
                 startTime=20140621T165241.938000
               currentTime=20140621T170712.007000
                    uptime=870 seconds
          nNameTreeEntries=8
               nFibEntries=2
               nPitEntries=2
      nMeasurementsEntries=0
                nCsEntries=24
              nInInterests=33
             nOutInterests=31
                  nInDatas=24
                 nOutDatas=18
    Channels:
      unix:///private/var/run/nfd.sock
      udp6://[::]:6363
      udp4://0.0.0.0:6363
      tcp6://[::]:6363
      tcp4://0.0.0.0:6363
    Faces:
      faceid=1 remote=internal:// local=internal:// counters={in={0i 26d} out={34i 0d}}
      faceid=2 remote=udp4://224.0.23.170:56363 local=udp4://192.168.1.103:56363 counters={in={0i 0d} out={0i 0d}}
      faceid=3 remote=udp4://224.0.23.170:56363 local=udp4://127.0.0.1:56363 counters={in={0i 0d} out={0i 0d}}
      faceid=4 remote=ether://[01:00:5e:00:17:aa] local=dev://bridge0 counters={in={0i 0d} out={0i 0d}}
      faceid=5 remote=ether://[01:00:5e:00:17:aa] local=dev://en0 counters={in={0i 0d} out={0i 0d}}
      faceid=6 remote=ether://[01:00:5e:00:17:aa] local=dev://en1 counters={in={0i 0d} out={0i 0d}}
      faceid=7 remote=fd://27 local=unix:///private/var/run/nfd.sock counters={in={23i 0d} out={0i 8d}}
      faceid=10 remote=fd://28 local=unix:///private/var/run/nfd.sock counters={in={3i 0d} out={0i 2d}}
    FIB:
      /localhost/nfd nexthops={faceid=1 (cost=0)}
      /localhost/nfd/rib nexthops={faceid=7 (cost=0)}
    Strategy choices:
      / strategy=/localhost/nfd/strategy/best-route
