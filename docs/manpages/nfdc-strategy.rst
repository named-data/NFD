nfdc-strategy
=============

SYNOPSIS
--------
| nfdc strategy [list]
| nfdc set-strategy <prefix> <strategy>
| nfdc unset-strategy <prefix>

DESCRIPTION
-----------
In NFD, packet forwarding behavior is determined by forwarding pipelines and forwarding strategies.
The forwarding pipelines define general steps of packet processing.
Forwarding strategies provide the intelligence to make decision on whether, when, and where
to forward Interests.

NFD contains multiple forwarding strategy implementations.
The strategy choice table determines which strategy is used in forwarding an Interest.

The **nfdc strategy list** command shows the strategy choices.

The **nfdc set-strategy** command sets the strategy for a name prefix.

The **nfdc unset-strategy** command clears the strategy choice at a name prefix,
so that a strategy choice at a shorter prefix or the default strategy will be used.
It undoes a prior **nfdc set-strategy** command on the same name prefix.

OPTIONS
-------
<prefix>
    The name prefix of a strategy choice.
    The strategy choice is effective for all Interests under the name prefix,
    unless overridden by another strategy choice.
    Specifying ``ndn:/`` as the prefix in **nfdc set-strategy** changes the default strategy.
    Specifying ``ndn:/`` as the prefix in **nfdc unset-strategy** is disallowed,
    because NFD must always have a default strategy.

<strategy>
    A name that identifies the forwarding strategy.
    Consult NFD Developer's Guide for a complete list of all implemented strategies.

SEE ALSO
--------
nfd(1), nfdc(1)
