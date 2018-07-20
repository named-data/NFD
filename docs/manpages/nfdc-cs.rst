nfdc-cs
=======

SYNOPSIS
--------
| nfdc cs [info]
| nfdc cs config [capacity <CAPACITY>] [admit on|off] [serve on|off]
| nfdc cs erase <PREFIX> [count <COUNT>]

DESCRIPTION
-----------
The **nfdc cs info** command shows CS statistics information.

The **nfdc cs config** command updates CS configuration.

The **nfdc cs erase** command erases cached Data under a name prefix.

OPTIONS
-------
<CAPACITY>
    Maximum number of Data packets the CS can store.
    Lowering the capacity causes the CS to evict excess Data packets.

admit on|off
    Whether the CS can admit new Data.

serve on|off
    Whether the CS can satisfy incoming Interests using cached Data.
    Turning this off causes all CS lookups to miss.

<PREFIX>
    Name prefix of cached Data packets.

<COUNT>
    Maximum number of cached Data packets to erase.
    The default is "no limit".

SEE ALSO
--------
nfd(1), nfdc(1)
