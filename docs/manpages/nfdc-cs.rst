nfdc-cs
=======

Synopsis
--------

| **nfdc cs** [**info**]
| **nfdc cs** **config** [**capacity** *CAPACITY*] [**admit** **on**\|\ **off**] [**serve** **on**\|\ **off**]
| **nfdc cs** **erase** *PREFIX* [**count** *COUNT*]

Description
-----------

The **nfdc cs info** command shows CS statistics information.

The **nfdc cs config** command updates CS configuration.

The **nfdc cs erase** command erases cached Data under a name prefix.

Options
-------

.. option:: <CAPACITY>

    Maximum number of Data packets the CS can store.
    Lowering the capacity causes the CS to evict excess Data packets.

.. option:: admit on|off

    Whether the CS can admit new Data.

.. option:: serve on|off

    Whether the CS can satisfy incoming Interests using cached Data.
    Turning this off causes all CS lookups to miss.

.. option:: <PREFIX>

    Name prefix of cached Data packets.

.. option:: <COUNT>

    Maximum number of cached Data packets to erase.
    The default is "no limit".

See Also
--------

:manpage:`nfdc(1)`
