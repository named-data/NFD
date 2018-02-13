nfdc-strategy
=============

SYNOPSIS
--------
| nfdc strategy [list]
| nfdc strategy show [prefix] <PREFIX>
| nfdc strategy set [prefix] <PREFIX> [strategy] <STRATEGY>
| nfdc strategy unset [prefix] <PREFIX>

DESCRIPTION
-----------
In NFD, packet forwarding behavior is determined by forwarding pipelines and forwarding strategies.
The forwarding pipelines define general steps of packet processing.
Forwarding strategies provide the intelligence to make decision on whether, when, and where
to forward Interests.

NFD contains multiple forwarding strategy implementations.
The strategy choice table determines which strategy is used in forwarding an Interest.

The **nfdc strategy list** command shows a list of strategy choices.

The **nfdc strategy show** command shows the effective strategy choice for a specific name.

The **nfdc strategy set** command sets the strategy for a name prefix.
The strategy for ``"/"`` prefix is the default strategy.

The **nfdc strategy unset** command clears the strategy choice at a name prefix,
so that a strategy choice at a shorter prefix or the default strategy will be used.
It undoes a prior **nfdc strategy set** command on the same name prefix.
It is prohibited to unset the strategy choice for ``"/"`` prefix because there must always be a
default strategy.

OPTIONS
-------
<PREFIX>
    The name prefix of a strategy choice.
    The strategy choice is effective for all Interests under the name prefix,
    unless overridden by another strategy choice at a longer prefix.

<STRATEGY>
    A name that identifies the forwarding strategy.
    Consult NFD Developer's Guide for a complete list of all implemented strategies.

EXIT CODES
----------
0: Success

1: An unspecified error occurred

2: Malformed command line

7: Strategy not found (**nfdc strategy set** only)

EXAMPLES
--------
nfdc strategy list
    List all configured strategy choices.

nfdc strategy show prefix /localhost/ping/1
    Identify which strategy will handle Interests named "/localhost/ping/1".

nfdc strategy set prefix / strategy /localhost/nfd/strategy/best-route
    Set the default strategy to best-route, latest version.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/multicast/%FD%01
    Set the strategy of the "/ndn" prefix to multicast, version 1.

nfdc strategy unset prefix /ndn
    Clear the strategy choice for the "/ndn" prefix.

SEE ALSO
--------
nfd(1), nfdc(1), nfd-asf-strategy(7)
