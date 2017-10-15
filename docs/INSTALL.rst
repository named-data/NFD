Getting Started with NFD
========================

Installing NFD from Binaries
----------------------------

We provide NFD binaries for the supported platforms, which are the preferred installation
method. In addition to simplifying installation, the binary release also includes automatic
initial configuration and platform-specific tools to automatically start NFD and related
daemons.  In particular, on macOS the binary release of NFD comes as part of `NDN Control
Center <https://named-data.net/codebase/applications/ndn-control-center/>`__ and provides a
convenient way to configure and launch NFD daemon.  :ref:`PPA packages for Ubuntu <Install NFD
Using the NDN PPA Repository on Ubuntu Linux>` include ``upstart`` and ``systemd``
configuration to automatically start daemon after boot.

Besides officially supported platforms, NFD is known to work on: Fedora 20+, CentOS 6+, Gentoo
Linux, Raspberry Pi, OpenWRT, FreeBSD 10+, and several `other platforms
<https://redmine.named-data.net/projects/nfd/wiki/Wiki#Installation-experiences-for-selected-platforms>`__.
We would also appreciate feedback and help packaging NDN releases for other platforms.

.. _Install NFD Using the NDN PPA Repository on Ubuntu Linux:

Install NFD Using the NDN PPA Repository on Ubuntu Linux
--------------------------------------------------------

NFD binaries and related tools for Ubuntu 14.04 and 16.04 can be installed using PPA
packages from named-data repository.  First, you will need to add ``named-data/ppa``
repository to binary package sources and update list of available packages.

Preliminary steps if you have not used PPA packages before
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To simplify adding new PPA repositories, Ubuntu provides ``add-apt-repository`` tool,
which is not installed by default on some systems.

::

    sudo apt-get install software-properties-common

Adding NDN PPA
~~~~~~~~~~~~~~

After installing ``add-apt-repository``, run the following command to add `NDN PPA
repository`_.

::

    sudo add-apt-repository ppa:named-data/ppa
    sudo apt-get update

Installing NFD and other NDN packages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After you have added `NDN PPA repository`_, NFD and other NDN packages can be easily
installed in a standard way, i.e., either using ``apt-get`` as shown below or using any
other package manager, such as Synaptic Package Manager:

::

    sudo apt-get install nfd

For the list of available packages, refer to `NDN PPA repository`_ homepage.

.. _NDN PPA repository: https://launchpad.net/~named-data/+archive/ppa

Building from Source
--------------------

Downloading from Git
~~~~~~~~~~~~~~~~~~~~

The first step is to obtain the source code for ``NFD`` and, its main dependency, the
``ndn-cxx`` library.  If you are not planning to work with the bleeding edge code, make
sure you checkout the correct release tag (e.g., ``*-0.6.0``) for both repositories:

::

    # Download ndn-cxx
    git clone https://github.com/named-data/ndn-cxx

    # Download NFD
    git clone --recursive https://github.com/named-data/NFD

.. note::
   While we strive to ensure that the latest version (master branch) of NFD and ndn-cxx
   always properly compiles and works, sometimes there could be problems.  In these cases, use
   the latest released version.

Prerequisites
~~~~~~~~~~~~~

Install the `ndn-cxx library <https://named-data.net/doc/ndn-cxx/current/INSTALL.html>`__ and its requirements

-  On macOS with Homebrew:

   ::

      brew install boost openssl pkg-config

- On Ubuntu:

   ::

       sudo apt-get install build-essential pkg-config libboost-all-dev \
                            libsqlite3-dev libssl-dev libpcap-dev

To build manpages and API documentation:

- On macOS with Homebrew:

   ::

       brew install doxygen graphviz
       sudo easy_install pip
       sudo pip Sphinx

- On Ubuntu:

   ::

       sudo apt-get install doxygen graphviz python-sphinx

Build
~~~~~

The following basic commands should be used to build NFD on Ubuntu and macOS with Homebrew:

::

    ./waf configure
    ./waf
    sudo ./waf install

If you have installed ``ndn-cxx`` library and/or other dependencies into a non-standard path, you
may need to modify ``PKG_CONFIG_PATH`` environment variable before running ``./waf configure``.
For example,

::

    export PKG_CONFIG_PATH=/custom/lib/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

Refer to ``./waf --help`` for more options that can be used during ``configure`` stage and
how to properly configure and run NFD.

.. note::
   If you are working on a source repository that has been compiled before, and you have
   upgraded one of the dependencies, please execute ``./waf distclean`` to clear object files
   and start over.

Debug symbols
~~~~~~~~~~~~~

The default compiler flags enable debug symbols to be included in binaries.  This
potentially allows more meaningful debugging if NFD or other tools happen to crash.

If it is undesirable, default flags can be easily overridden.  The following example shows
how to completely disable debug symbols and configure NFD to be installed into ``/usr``
with configuration in ``/etc`` folder.

::

    CXXFLAGS="-O2" ./waf configure --prefix=/usr --sysconfdir=/etc
    ./waf
    sudo ./waf install

.. note::
   For Ubuntu PPA packages debug symbols are available in ``*-dbg`` packages.

Customize Compiler
~~~~~~~~~~~~~~~~~~

To choose a custom C++ compiler for building NFD, set the ``CXX`` environment variable
to point to the compiler binary. For example, to select the clang compiler on a Linux
system, use the following:

::

    CXX=clang++ ./waf configure

Building documentation
~~~~~~~~~~~~~~~~~~~~~~

NFD tutorials and API documentation can be built using the following commands:

::

    # Full set of documentation (tutorials + API) in build/docs
    ./waf docs

    # Only tutorials in `build/docs`
    ./waf sphinx

    # Only API docs in `build/docs/doxygen`
    ./waf doxgyen


Manpages are automatically created and installed during the normal build process (e.g.,
during ``./waf`` and ``./waf install``), if ``python-sphinx`` module is detected during
``./waf configure`` stage.  By default, manpages are installed into
``${PREFIX}/share/man`` (where default value for ``PREFIX`` is ``/usr/local``). This
location can be changed during ``./waf configure`` stage using ``--prefix``,
``--datarootdir``, or ``--mandir`` options.

For more details, refer to ``./waf --help``.

Initial configuration
---------------------

.. note::
    If you have installed NFD from binary packages, the package manager has already
    installed initial configuration and you can safely skip this section.

General
~~~~~~~

After installing NFD from source, you need to create a proper config file.  If default
location for ``./waf configure`` was used, this can be accomplished by simply copying the
sample configuration file:

::

    sudo cp /usr/local/etc/ndn/nfd.conf.sample /usr/local/etc/ndn/nfd.conf

NFD Security
~~~~~~~~~~~~

NFD provides mechanisms to enable strict authorization for all management commands. In
particular, one can authorize only specific public keys to create new Faces or change the
forwarding strategy for specific namespaces. For more information about how to generate
private/public key pair, generate self-signed certificate, and use this self-signed
certificate to authorize NFD management commands, refer to :ref:`How to configure NFD
security` FAQ question.

In the sample configuration file, all authorizations are disabled, effectively allowing
anybody on the local machine to issue NFD management commands. **The sample file is
intended only for demo purposes and MUST NOT be used in a production environment.**

Running
-------

Starting
~~~~~~~~

If you have installed NFD from source code, it is recommended to start NFD with the
``nfd-start`` script:

::

    nfd-start

On macOS it may ask for your keychain password or ask ``nfd wants to sign using key in
your keychain.`` Enter your keychain password and click Always Allow.

Later, you can stop NFD with ``nfd-stop`` or by simply killing the ``nfd`` process.

If you have installed NFD using a package manager, you can start and stop NFD service using the
operating system's service manager (such as Upstart, systemd, or launchd) or using
"Automatically start NFD" option in NDN Control Center app.

Connecting to remote NFDs
~~~~~~~~~~~~~~~~~~~~~~~~~

To create a UDP tunnel to a remote NFD, execute the following command in terminal:

::

    nfdc face create udp://<other host>

where ``<other host>`` is the name or IP address of the other host (e.g.,
``udp://spurs.cs.ucla.edu``). This outputs:

::

    face-created id=308 local=udp4://10.0.2.15:6363 remote=udp4://131.179.196.46:6363 persistency=persistent

To add a route ``/ndn`` toward the remote NFD, execute the following command in terminal:

::

    nfdc route add /ndn udp://<other host>

This outputs:

::

    route-add-accepted prefix=/ndn nexthop=308 origin=static cost=0 flags=child-inherit expires=never

The ``/ndn`` means that NFD will forward all Interests that start with ``/ndn`` through the
face to the other host.  If you only want to forward Interests with a certain prefix, use it
instead of ``/ndn``.  This only forwards Interests to the other host, but there is no "back
route" for the other host to forward Interests to you.  For that, you can rely on automatic
prefix propagation feature of NFD or go to the other host and use ``nfdc`` to add the route.

Playing with NFD
----------------

After you haved installed, configured, and started NFD, you can try to install and play
with the following:

Sample applications:

- `Simple examples in ndn-cxx library <https://named-data.net/doc/ndn-cxx/current/examples.html>`_

   If you have installed ndn-cxx from source, you already have compiled these:

   +  examples/producer
   +  examples/consumer
   +  examples/consumer-with-timer

- `Introductory examples of NDN-CCL
  <https://redmine.named-data.net/projects/application-development-documentation-guides/wiki/Step-By-Step_-_Common_Client_Libraries>`_

Real applications and libraries:

   + `ndn-tools - NDN Essential Tools <https://github.com/named-data/ndn-tools>`_
   + `ndn-traffic-generator - Traffic Generator For NDN
     <https://github.com/named-data/ndn-traffic-generator>`_
   + `repo-ng - Next generation of NDN repository <https://github.com/named-data/repo-ng>`_
   + `ChronoChat - Multi-user NDN chat application <https://github.com/named-data/ChronoChat>`_
   + `ChronoSync - Sync library for multiuser realtime applications for NDN
     <https://github.com/named-data/ChronoSync>`_
