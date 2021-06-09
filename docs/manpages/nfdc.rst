nfdc
====

SYNOPSIS
--------
| nfdc COMMAND [ARGUMENTS] ...
| nfdc help [COMMAND]
| nfdc [-h|--help]
| nfdc -V|--version
| nfdc -f|--batch BATCH-FILE

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

``-h`` or ``--help``
    Print a list of available subcommands and exit.

``-V`` or ``--version``
    Show version information and exit.

``-f BATCH-FILE`` or ``--batch BATCH-FILE``
   Process arguments specified in the ``BATCH-FILE`` as if they would have appeared
   in the command line (but without ``nfdc``).  When necessary, arguments should be
   escaped using backslash ``\``, single ``'``, or double quotes ``"``.  If any of
   the command fails, the processing will be stopped at that command with error
   code 2. Empty lines and lines that start with ``#`` character will be ignored.
   Note that the batch file does not support empty string arguments
   (``""`` or ``''``), even if they are supported by the regular command line ``nfdc``.

   When running a sequence of commands in rapid succession from a script, this
   option ensures that the commands are properly timestamped and can therefore
   be accepted by NFD.

EXAMPLES
--------
nfdc
    List all subcommands.

nfdc help face create
    Show how to use the ``nfdc face create`` subcommand.

SEE ALSO
--------
nfdc-status(1), nfdc-face(1), nfdc-route(1), nfdc-cs(1), nfdc-strategy(1)
