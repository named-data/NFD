.. _NFD Installation Instructions:

NFD Installation Instructions
=============================

Prerequisites
-------------

-  `ndn-cxx library <https://github.com/named-data/ndn-cxx>`__
   and its requirements:

   -  ``libcrypto``
   -  ``libsqlite3``
   -  ``libcrypto++``
   -  ``pkg-config``
   -  Boost libraries (>= 1.48)
   -  OSX Security framework (on OSX platform only)

   Refer to `Getting started with ndn-cxx <http://named-data.net/doc/ndn-cxx/current/INSTALL.html>`_
   for detailed installation instructions of the library.

-  ``libpcap``

   Comes with base on OS X 10.8 and 10.9:

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install libpcap-dev

To build manpages and API documentation:

-  ``doxygen``
-  ``graphviz``
-  ``python-sphinx``

   On OS X 10.8 and 10.9 with macports:

   ::

       sudo port install doxygen graphviz py27-sphinx sphinx_select
       sudo port select sphinx py27-sphinx

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install doxygen graphviz python-sphinx

Build
-----

The following commands should be used to build NFD:

::

    ./waf configure
    ./waf
    sudo ./waf install

Refer to ``./waf --help`` for more options that can be used during ``configure`` stage and
how to properly configure and run NFD.

In some configurations, configuration step may require small modification. For example, on
OSX that uses macports (correct the path if macports was not installed in the default path
``/opt/local``):

::

    export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

On some Linux distributions (e.g., Fedora 20):

::

    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

Debug symbols
+++++++++++++

The default compiler flags enable debug symbols to be included in binaries.  This
potentially allows more meaningful debugging if NFD or other tools happen to crash.

If it is undesirable, default flags can be easily overridden.  The following example shows
how to completely disable debug symbols and configure NFD to be installed into ``/usr``
with configuration in ``/etc`` folder.

::

    CXXFLAGS="-O2" ./waf configure --prefix=/usr --sysconfdir=/etc
    ./waf
    sudo ./waf install

Building documentation
----------------------

NFD tutorials and API documentation can be built using the following commands:

::

    # Full set of documentation (tutorials + API) in build/docs
    ./waf docs

    # Only tutorials in `build/docs`
    ./waf sphinx

    # Only API docs in `build/docs/doxygen`
    ./waf doxgyen


Manpages are automatically created and installed during the normal build process
(e.g., during ``./waf`` and ``./waf install``), if ``python-sphinx`` module is detected
during ``./waf configure`` stage.  By default, manpages are installed into
``${PREFIX}/share/man`` (where default value for ``PREFIX`` is ``/usr/local``). This
location can be changed during ``./waf configure`` stage using ``--prefix``,
``--datarootdir``, or ``--mandir`` options.

For more details, refer to ``./waf --help``.
