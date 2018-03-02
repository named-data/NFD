nfdc-cs
=======

SYNOPSIS
--------
| nfdc cs [info]
| nfdc cs config [capacity <CAPACITY>] [admit on|off] [serve on|off]

DESCRIPTION
-----------
The **nfdc cs info** command shows CS statistics information.

The **nfdc cs config** command updates CS configuration.

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

SEE ALSO
--------
nfd(1), nfdc(1)
