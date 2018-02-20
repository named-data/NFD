nfdc-route
==========

SYNOPSIS
--------
| nfdc route [list [[nexthop] <FACEID|FACEURI>] [origin <ORIGIN>]]
| nfdc route show [prefix] <PREFIX>
| nfdc route add [prefix] <PREFIX> [nexthop] <FACEID|FACEURI> [origin <ORIGIN>]
|                [cost <COST>] [no-inherit] [capture] [expires <EXPIRATION-MILLIS>]
| nfdc route remove [prefix] <PREFIX> [nexthop] <FACEID|FACEURI> [origin <ORIGIN>]
| nfdc fib [list]

DESCRIPTION
-----------
In NFD, the routing information base (RIB) stores static or dynamic routing information
registered by applications, operators, and NFD itself.
Each *route* in the RIB indicates that contents under a name prefix may be available via a nexthop.
A route contains a name prefix, a nexthop face, the origin, a cost, and a set of route inheritance flags;
refer to NFD Management protocol for more information.

The **nfdc route list** command lists RIB routes, optionally filtered by nexthop and origin.

The **nfdc route show** command shows RIB routes at a specified name prefix.

The **nfdc route add** command requests to add a route.
If a route with the same prefix, nexthop, and origin already exists,
it is updated with the specified cost, route inheritance flags, and expiration period.
This command returns when the request has been accepted, but does not wait for RIB update completion.

The **nfdc route remove** command removes a route with matching prefix, nexthop, and origin.

The **nfdc fib list** command shows the forwarding information base (FIB),
which is calculated from RIB routes and used directly by NFD forwarding.

OPTIONS
-------
<PREFIX>
    Name prefix of the route.

<FACEID>
    Numerical identifier of the face.
    It is displayed in the output of **nfdc face list** and **nfdc face create** commands.

<FACEURI>
    An URI representing the remote endpoint of a face.
    In **nfdc route add** command, it must uniquely match an existing face.
    In **nfdc route remove** command, it must match one or more existing faces.

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

3: Face not found

4: FaceUri canonization failed

5: Ambiguous: multiple matching faces are found (**nfdc route add** only)

6: Route not found (**nfdc route list** and **nfdc route show** only)

EXAMPLES
--------
nfdc route list
    List all routes.

nfdc route list nexthop 300
    List routes whose nexthop is face 300.

nfdc route list origin static
    List static routes.

nfdc route show prefix /localhost/nfd
    List routes with name prefix "/localhost/nfd".

nfdc route add prefix /ndn nexthop 300 cost 100
    Add a route with prefix "/ndn" toward face 300, with administrative cost 100.

nfdc route add prefix / nexthop udp://router.example.net
    Add a route with prefix "/" toward a face with the specified remote FaceUri.

nfdc route remove prefix /ndn nexthop 300 origin static
    Remove the route whose prefix is "/ndn", nexthop is face 300, and origin is "static".

SEE ALSO
--------
nfd(1), nfdc(1)
