nfdc-face
=========

SYNOPSIS
--------
| nfdc face [list]
| nfdc channel [list]
| nfdc create [-P] <faceUri>
| nfdc destroy <faceId|faceUri>

DESCRIPTION
-----------
In NFD, a face is the generalization of network interface.
It could be a physical network interface to communicate on a physical link,
an overlay communication channel between NFD and a remote node,
or an inter-process communication channel between NFD and a local application.

The **nfdc face list** command shows a list of faces, their properties, and statistics.

The **nfdc channel list** command shows a list of channels.
Channels are listening sockets that can accept incoming connections and create new faces.

The **nfdc create** command creates a new UDP or TCP face.

The **nfdc destroy** command destroys an existing face.
It has no effect if the specified face does not exist.

OPTIONS
-------
-P
    Creates a "permanent" rather than persistent face.
    A persistent face is closed when a socket error occrs.
    A permanent face is kept alive upon socket errors,
    and is closed only upon **nfdc destroy** command.

<faceUri>
    An URI representing the remote endpoint of a face.
    Its syntax is:

    - udp[4|6]://<IP-or-host>[:<port>]
    - tcp[4|6]://<IP-or-host>[:<port>]

    When a hostname is specified, a DNS query is used to obtain the IP address.

<faceId>
    Numerical identifier of the face.
    It is displayed in the output of **nfdc face list** and **nfdc create** commands.

SEE ALSO
--------
nfd(1), nfdc(1)
