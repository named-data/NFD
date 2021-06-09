# Manual test for nfdc batch mode

To run the test from this folder

    nfdc -f nfdc-batch.t.txt

    nfdc --batch nfdc-batch.t.txt

If everything works, it should execute 3 commands with example output like this
in both cases (can be different depending on the NFD runtime):

    face-exists id=263 local=udp4://192.168.100.240:6363 remote=udp4://192.0.2.1:6363 persistency=persistent reliability=off congestion-marking=on congestion-marking-interval=100ms default-congestion-threshold=65536B mtu=8800
    route-add-accepted prefix=/ nexthop=264 origin=static cost=0 flags=child-inherit expires=never
    route-add-accepted prefix=/test2/foo%20bar nexthop=265 origin=static cost=0 flags=child-inherit expires=never
    CS information:
      capacity=65536
         admit=on
         serve=on
      nEntries=14
         nHits=0
       nMisses=53
