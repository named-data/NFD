nfdc-route
==========

SYNOPSIS
--------
| nfdc route [list]
| nfdc fib [list]
| nfdc route add [prefix] <PREFIX> [nexthop] <FACEID|FACEURI> [origin <ORIGIN>] [cost <COST>]
|   [no-inherit] [capture] [expires <EXPIRATION-MILLIS>]
| nfdc unregister [-o <ORIGIN>] <PREFIX> <FACEID>

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

The **nfdc route add** command requests to add a route.
If a route with the same prefix, nexthop, and origin already exists,
it is updated with the specified cost, route inheritance flags, and expiration period.
This command returns when the request has been accepted, but does not wait for RIB update completion.

The **nfdc unregister** command removes a route with matching prefix, nexthop, and origin.

OPTIONS
-------
<PREFIX>
    Name prefix of the route.

<FACEID>
    Numerical identifier of the face.
    It is displayed in the output of **nfdc face list** and **nfdc face create** commands.

<FACEURI>
    An URI representing the remote endpoint of a face.
    It must uniquely match an existing face.

<ORIGIN>
    Origin of the route, i.e. who is announcing the route.
    The default is 255, indicating a static route.

<COST>
    The administrative cost of the route.
    The default is 0.

no-inherit
    Unset CHILD_INHERIT flag in the route.

capture
    Set CAPTURE flag in the route.

<EXPIRATION-MILLIS>
    Expiration time of the route, in milliseconds.
    When the route expires, NFD removes it from the RIB.
    The default is infinite, which keeps the route active until the nexthop face is destroyed.

EXIT CODES
----------

0: Success

1: An unspecified error occurred

2: Malformed command line

3: Face not found (**nfdc route add** only)

4: FaceUri canonization failed (**nfdc route add** only)

5: Ambiguous: multiple matching faces are found (**nfdc route add** only)

SEE ALSO
--------
nfd(1), nfdc(1)
