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

``-r``
  Retrieve RIB information.

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
                     nfdId=/chengyu/KEY/ksk-1405136377018/ID-CERT
                   version=2000
                 startTime=20140725T232341.374000
               currentTime=20140725T233240
                    uptime=538 seconds
          nNameTreeEntries=10
               nFibEntries=3
               nPitEntries=2
      nMeasurementsEntries=0
                nCsEntries=56
              nInInterests=55
             nOutInterests=54
                  nInDatas=56
                 nOutDatas=47
    Channels:
      ws://[::]:9696
      unix:///private/var/run/nfd.sock
      udp6://[::]:6363
      udp4://0.0.0.0:6363
      tcp6://[::]:6363
      tcp4://0.0.0.0:6363
    Faces:
      faceid=1 remote=internal:// local=internal:// counters={in={0i 52d 0B} out={51i 0d 0B}} local
      faceid=254 remote=contentstore:// local=contentstore:// counters={in={0i 0d 0B} out={0i 0d 0B}} local
      faceid=255 remote=null:// local=null:// counters={in={0i 0d 0B} out={0i 0d 0B}} local
      faceid=256 remote=udp4://224.0.23.170:56363 local=udp4://129.82.138.211:56363 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=257 remote=udp4://224.0.23.170:56363 local=udp4://127.0.0.1:56363 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=258 remote=ether://[01:00:5e:00:17:aa] local=dev://bridge0 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=259 remote=ether://[01:00:5e:00:17:aa] local=dev://en0 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=260 remote=ether://[01:00:5e:00:17:aa] local=dev://en1 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=261 remote=ether://[01:00:5e:00:17:aa] local=dev://en2 counters={in={0i 0d 0B} out={0i 0d 0B}}
      faceid=262 remote=fd://30 local=unix:///private/var/run/nfd.sock counters={in={24i 6d 4880B} out={6i 16d 8417B}} local on-demand
      faceid=268 remote=fd://31 local=unix:///private/var/run/nfd.sock counters={in={1i 0d 410B} out={0i 1d 764B}} local on-demand
      faceid=269 remote=fd://32 local=unix:///private/var/run/nfd.sock counters={in={3i 0d 137B} out={0i 2d 925B}} local on-demand
    FIB:
      /localhost/nfd nexthops={faceid=1 (cost=0)}
      /example/testApp nexthops={faceid=268 (cost=0)}
      /localhost/nfd/rib nexthops={faceid=262 (cost=0)}
    RIB:
      /example/testApp route={faceid=268 (origin=0 cost=0 flags=1)}
    Strategy choices:
      / strategy=/localhost/nfd/strategy/best-route
