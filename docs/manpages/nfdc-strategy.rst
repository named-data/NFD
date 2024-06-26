nfdc-strategy
=============

Synopsis
--------

| **nfdc strategy** [**list**]
| **nfdc strategy** **show** [**prefix**] *PREFIX*
| **nfdc strategy** **set** [**prefix**] *PREFIX* [**strategy**] *STRATEGY*
| **nfdc strategy** **unset** [**prefix**] *PREFIX*

Description
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
The strategy for the ``"/"`` prefix is the default strategy.

The **nfdc strategy unset** command clears the strategy choice at a name prefix,
so that a strategy choice at a shorter prefix or the default strategy will be used.
It undoes a prior **nfdc strategy set** command on the same name prefix.
It is prohibited to unset the strategy choice for the ``"/"`` prefix because there must always be
a default strategy.

Options
-------

.. option:: <PREFIX>

    The name prefix of a strategy choice.
    The strategy choice is effective for all Interests under the name prefix,
    unless overridden by another strategy choice at a longer prefix.

.. option:: <STRATEGY>

    A name that identifies the forwarding strategy.
    Consult the NFD Developer's Guide for a complete list of all implemented strategies.

    For the ASF, BestRoute, and Multicast strategies, the following options may be supplied
    to configure exponential retransmission suppression by appending them after the strategy
    name and version number.

    retx-suppression-initial
        Starting duration of the suppression interval within which any retransmitted
        Interests are considered for suppression. The default value is 10 ms.

        Format: ``retx-suppression-initial~<milliseconds>``

    retx-suppression-max
        Maximum duration of the suppression interval. Must not be smaller than
        **retx-suppression-initial**. The default value is 250 ms.

        Format: ``retx-suppression-max~<milliseconds>``

    retx-suppression-multiplier
        The suppression interval is increased by this multiplier. The default value is 2.

        Format: ``retx-suppression-multiplier~<float>``

    See :manpage:`nfd-asf-strategy(7)` for details on additional parameters for ASF strategy.

Exit Status
-----------

0
    Success.

1
    An unspecified error occurred.

2
    Malformed command line.

7
    Strategy not found (**nfdc strategy set** only).

Examples
--------

``nfdc strategy list``
    List all configured strategy choices.

``nfdc strategy show prefix /localhost/ping/1``
    Identify which strategy will handle Interests named "/localhost/ping/1".

``nfdc strategy set prefix / strategy /localhost/nfd/strategy/best-route``
    Set the default strategy to best-route, latest version.

``nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/multicast/v=4``
    Set the strategy for the "/ndn" prefix to multicast, version 4.

``nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/multicast/v=4/retx-suppression-initial~20/retx-suppression-max~500``
    Set the strategy for the "/ndn" prefix to multicast, version 4, with retransmission
    suppression initial interval set to 20 ms and maximum interval set to 500 ms.

``nfdc strategy unset prefix /ndn``
    Clear the strategy choice for the "/ndn" prefix.

See Also
--------

:manpage:`nfdc(1)`,
:manpage:`nfd-asf-strategy(7)`
