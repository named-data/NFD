.. _ndn-autoconfig.conf:

autoconfig.conf
===============

Overview
--------

Configuration of NDN auto-configuration client daemon in INI format.  Comments in the
configuration file must be prefixed with ``#``.

Example of ``autoconfig.conf``:

.. literalinclude:: ../../autoconfig.conf.sample

Options
-------

``enabled``
  (default: false) Enable or disable automatic NDN auto-configuration client daemon.

  When enabled, NDN auto-configuration client daemon is automatically started by ``nfd-start``.
