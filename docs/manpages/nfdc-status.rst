nfdc-status
===========

SYNOPSIS
--------
| nfdc status [show]
| nfdc status report [<FORMAT>]

DESCRIPTION
-----------
The **nfdc status show** command shows general status of NFD, including its version,
uptime, data structure counters, and global packet counters.

The **nfdc status report** command prints a comprehensive report of NFD status, including:

- general status (individually available from **nfdc status show**)
- list of channels (individually available from **nfdc channel list**)
- list of faces (individually available from **nfdc face list**)
- list of FIB entries (individually available from **nfdc fib list**)
- list of RIB entries (individually available from **nfdc route list**)
- CS statistics information (individually available from **nfdc cs info**)
- list of strategy choices (individually available from **nfdc strategy list**)

OPTIONS
-------
<FORMAT>
    The format of NFD status report, either ``text`` or ``xml``.
    The default is ``text``.

SEE ALSO
--------
nfdc(1), nfdc-channel(1), nfdc-face(1), nfdc-fib(1), nfdc-route(1), nfdc-strategy(1)
