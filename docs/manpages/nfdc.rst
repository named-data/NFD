nfdc
====

Synopsis
--------

| **nfdc** *COMMAND* [*ARGUMENTS*]...
| **nfdc** **help** [*COMMAND*]
| **nfdc** [**-h**\|\ **\--help**]
| **nfdc** **-V**\|\ **\--version**
| **nfdc** **-f**\|\ **\--batch** *file*

Description
-----------

:program:`nfdc` is a command-line tool to manage a running instance of NFD.

All features of :program:`nfdc` are organized into subcommands.
To print a list of all subcommands, run ``nfdc`` without arguments.
To show how to use a subcommand, run ``nfdc help`` followed by the subcommand name.

Options
-------

.. option:: <COMMAND>

    A subcommand name. It usually contains a noun and a verb.

.. option:: <ARGUMENTS>

    Arguments to the subcommand.

.. option:: -h, --help

    Print a list of available subcommands and exit.

.. option:: -V, --version

    Show version information and exit.

.. option:: -f <file>, --batch <file>

    Process the list of arguments specified in *file* as if they would have appeared on
    the command line.
    When running a sequence of commands in rapid succession from a script, this option
    ensures that the commands are properly timestamped and can therefore be accepted
    by NFD.

    When necessary, arguments should be escaped using a backslash (``\``), single quotes
    (``'``), or double quotes (``"``).
    If any of the commands fail, processing will stop at that command and :program:`nfdc`
    will exit with error code 2.
    Empty lines and lines that start with the ``#`` character are ignored.
    Note that the batch file does not support empty string arguments (``""`` or ``''``),
    even if they would normally be supported on the regular command line of :program:`nfdc`.

Examples
--------

``nfdc``
    List all subcommands.

``nfdc help face create``
    Show how to use the ``nfdc face create`` subcommand.

See Also
--------

:manpage:`nfd(1)`,
:manpage:`nfdc-cs(1)`,
:manpage:`nfdc-face(1)`,
:manpage:`nfdc-route(1)`,
:manpage:`nfdc-status(1)`,
:manpage:`nfdc-strategy(1)`
