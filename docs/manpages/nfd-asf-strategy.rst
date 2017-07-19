nfd-asf-strategy
================

SYNOPSIS
--------
| nfdc strategy set prefix <PREFIX> strategy /localhost/nfd/strategy/asf/%FD%02[/probing-interval~<PROBING-INTERVAL>][/n-silent-timeouts~<N-SILENT-TIMEOUTS>]

DESCRIPTION
-----------

ASF is an Adaptive Smoothed RTT-based Forwarding Strategy that chooses the best next hop based on SRTT measurement, and also periodically probes other next hops to learn their RTTs.

OPTIONS
-------
<PROBING-INTERVAL>
    Tells ASF how often to send a probe to determine alternative paths.
    The value is specified in milliseconds (non-negative integer)
    Lower value means high overhead but faster reaction.
    Default value is 1 minute and minimum value is 1 second.
    It is optional to specify probing-interval.

<N-SILENT-TIMEOUTS>
    ASF switches immediately to another appropriate face (if available) upon timeout.
    This behavior may be too sensitive for application use and appropriate only for link
    failures and not transient timeouts. So this parameter makes ASF switch paths
    only after it has encountered the specified number of timeouts (non-negative integer).
    Default and minimum value is 0 i.e. switch immediately.
    It is optional to specify n-silent-timeouts.

EXAMPLES
--------
nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf
    Use the default values.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/%FD%03/probing-interval~30000
    Set probing interval as 30 seconds.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/%FD%03/n-silent-timeouts~5
    Set n-silent-timeouts as 5.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/%FD%03/probing-interval~30000/n-silent-timeouts~5
    Set probing interval as 30 seconds and n-silent-timeouts as 5.

SEE ALSO
--------
nfdc(1), nfdc-strategy(1)