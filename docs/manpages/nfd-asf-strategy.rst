nfd-asf-strategy
================

Synopsis
--------

**nfdc strategy set** **prefix** *NAME* **strategy**
/localhost/nfd/strategy/asf[/v=4][/**probing-interval**\ ~\ *INTERVAL*][/**max-timeouts**\ ~\ *TIMEOUTS*]

Description
-----------

**ASF** is an Adaptive Smoothed RTT-based Forwarding Strategy that chooses the
best next hop based on SRTT measurements, and also periodically probes other
next hops to learn their RTTs.

Options
-------

.. option:: probing-interval

    This optional parameter tells ASF how often to send a probe to determine
    alternative paths. The value is specified in milliseconds (non-negative
    integer). Smaller values will result in higher overhead but faster reaction.
    The default value is 1 minute and the minimum value is 1 second.

.. option:: max-timeouts

    This optional parameter makes ASF switch to another appropriate face (if available)
    after it has encountered the specified number of timeouts. The value is a positive
    integer and defaults to 3, i.e., switch to another face after 3 timeouts. Smaller
    values make ASF more sensitive to timeouts and will switch paths more frequently,
    which should provide a faster reaction to link failures. Larger values may be better
    suited when transient timeouts are common and for certain application uses.

Examples
--------

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf
    Use the default values for all parameters.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/v=4/probing-interval~30000
    Set the probing interval to 30 seconds.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/v=4/max-timeouts~5
    Set the maximum number of timeouts to 5.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/v=4/probing-interval~30000/max-timeouts~2
    Set the probing interval to 30 seconds and the maximum number of timeouts to 2.

nfdc strategy set prefix /ndn strategy /localhost/nfd/strategy/asf/v=4/retx-suppression-multiplier~2.5/probing-interval~45000
    Set the retransmission suppression multiplier to 2.5 and the probing interval
    to 45 seconds. See :manpage:`nfdc-strategy(1)` for more information on the
    retransmission suppression parameters.

See also
--------

nfdc(1), nfdc-strategy(1)
