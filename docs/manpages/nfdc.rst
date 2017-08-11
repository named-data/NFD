nfdc
====

SYNOPSIS
--------
| nfdc COMMAND ARGUMENTS
| nfdc [-h]
| nfdc -V
| nfdc help COMMAND

DESCRIPTION
-----------
**nfdc** is a tool to manage a running instance of NFD.

Features of nfdc are organized into subcommands.
To print a list of all subcommands, run **nfdc** without arguments.
To show how to use a subcommand, run **nfdc help** followed by the subcommand name.

OPTIONS
-------
<COMMAND>
    A subcommand name.
    It usually contains a noun and a verb.

<ARGUMENTS>
    Arguments to the subcommand.

-h
    Print a list of available subcommands.

-V
    Print version number.

EXAMPLES
--------
Print a list of all subcommands:
::

    $ nfdc

Show how to use ``nfdc face create`` command:
::

    $ nfdc help face create

SEE ALSO
--------
nfdc-status(1), nfdc-face(1), nfdc-route(1), nfdc-strategy(1)
