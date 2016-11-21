nfdc-route
==========

SYNOPSIS
--------
| nfdc route [list]
| nfdc fib [list]
| nfdc register [-I] [-C] [-c <cost>] [-e <expiration>] [-o <origin>] <prefix> <faceId|faceUri>
| nfdc unregister [-o <origin>] <prefix> <faceId>

DESCRIPTION
-----------
In NFD, the routing information base (RIB) stores static or dynamic routing information
registered by applications, operators, and NFD itself.
Each *route* in the RIB indicates that contents under a name prefix may be available via a nexthop.
A route contains a name prefix, a nexthop face, the origin, a cost, and a set of route inheritance flags;
refer to NFD Management protocol for more information.

The **nfdc route list** command shows a list of routes in the RIB.

The **nfdc fib list** command shows the forwarding information base (FIB),
which is calculated from RIB routes and used directly by NFD forwarding.

The **nfdc register** command adds a new route.
If a route with the same prefix, nexthop, and origin already exists,
it is updated with the specified cost, expiration, and route inheritance flags.

The **nfdc unregister** command removes a route with matching prefix, nexthop, and origin.

OPTIONS
-------
-I
    Unset CHILD_INHERIT flag in the route.

-C
    Set CAPTURE flag in the route.

-c <cost>
    The administrative cost of the route.
    The default is 0.

-e <expiration>
    Expiration time of the route, in milliseconds.
    When the route expires, NFD removes it from the RIB.
    The default is infinite, which keeps the route active until the nexthop face is destroyed.

-o <origin>
    Origin of the route, i.e. who is announcing the route.
    The default is 255, indicating a static route.

<prefix>
    Name prefix of the route.

<faceUri>
    An URI representing the remote endpoint of a face.
    It can be used in **nfdc register** to create a new UDP or TCP face.

<faceId>
    A numerical identifier of the face.
    It is displayed in the output of **nfdc face list** and **nfdc create** commands.

SEE ALSO
--------
nfd(1), nfdc(1)
