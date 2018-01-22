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
nfdc
    List all subcommands.

nfdc help face create
    Show how to use the ``nfdc face create`` subcommand.

SEE ALSO
--------
nfdc-status(1), nfdc-face(1), nfdc-route(1), nfdc-cs(1), nfdc-strategy(1)
