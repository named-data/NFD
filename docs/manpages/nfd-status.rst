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

Exit Codes
----------

0: Success

1: An unspecified error occurred

2: Malformed command line

Examples
--------

Get all status information from NFD::

    $ nfd-status

    General NFD status:
                     nfdId=/tmp-identity/%80%80%BF%9A/KEY/ksk-1457726482439/ID-CERT
                   version=0.4.1-10-g4f1afac
                 startTime=20160426T224102.791000
               currentTime=20160426T224108.813000
                    uptime=6 seconds
          nNameTreeEntries=11
               nFibEntries=2
               nPitEntries=2
      nMeasurementsEntries=0
                nCsEntries=2
              nInInterests=5
             nOutInterests=5
                  nInDatas=7
                 nOutDatas=4
                  nInNacks=0
                 nOutNacks=0
    Channels:
      tcp4://0.0.0.0:6363
      tcp6://[::]:6363
      udp4://0.0.0.0:6363
      udp6://[::]:6363
      unix:///private/var/run/nfd.sock
      ws://[::]:9696
    Faces:
      faceid=1 remote=internal:// local=internal:// counters={in={0i 9d 0n 4830B} out={8i 0d 0n 1232B}} local permanent point-to-point
      faceid=254 remote=contentstore:// local=contentstore:// counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} local permanent point-to-point
      faceid=255 remote=null:// local=null:// counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} local permanent point-to-point
      faceid=256 remote=udp4://224.0.23.170:56363 local=udp4://10.134.195.206:56363 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=257 remote=udp4://224.0.23.170:56363 local=udp4://127.0.0.1:56363 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=258 remote=ether://[01:00:5e:00:17:aa] local=dev://awdl0 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=259 remote=ether://[01:00:5e:00:17:aa] local=dev://en2 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=260 remote=ether://[01:00:5e:00:17:aa] local=dev://en1 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=261 remote=ether://[01:00:5e:00:17:aa] local=dev://en0 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=262 remote=ether://[01:00:5e:00:17:aa] local=dev://bridge0 counters={in={0i 0d 0n 0B} out={0i 0d 0n 0B}} non-local permanent multi-access
      faceid=263 remote=fd://36 local=unix:///private/var/run/nfd.sock counters={in={5i 0d 0n 998B} out={0i 4d 0n 2511B}} local on-demand point-to-point
      faceid=264 remote=fd://37 local=unix:///private/var/run/nfd.sock counters={in={3i 0d 0n 146B} out={0i 2d 0n 992B}} local on-demand point-to-point
    FIB:
      /localhost/nfd/rib nexthops={faceid=263 (cost=0)}
      /localhost/nfd nexthops={faceid=1 (cost=0)}
    RIB:
      /localhost/nfd/rib route={faceid=263 (origin=0 cost=0 ChildInherit)}
    Strategy choices:
      / strategy=/localhost/nfd/strategy/best-route/%FD%04
      /localhost strategy=/localhost/nfd/strategy/multicast/%FD%01
      /ndn/broadcast strategy=/localhost/nfd/strategy/multicast/%FD%01
      /localhost/nfd strategy=/localhost/nfd/strategy/best-route/%FD%04
