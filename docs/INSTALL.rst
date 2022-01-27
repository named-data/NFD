Getting started with NFD
========================

Supported platforms
-------------------

NFD is built against a continuous integration system and has been tested on the
following platforms:

-  Ubuntu 18.04 (amd64, armhf, i386)
-  Ubuntu 20.04 (amd64)
-  Ubuntu 21.10 (amd64)
-  macOS 10.15
-  macOS 11 (Intel only)
-  CentOS 8

NFD is known to work on the following platforms, although they are not officially
supported:

-  Alpine >= 3.12
-  Debian >= 10
-  Fedora >= 29
-  Gentoo Linux
-  Raspberry Pi OS (formerly Raspbian) >= 2019-06-20
-  macOS 10.14

.. _Install NFD on Ubuntu Linux using the NDN PPA repository:

Install NFD on Ubuntu Linux using the NDN PPA repository
--------------------------------------------------------

NFD binaries and related tools for supported versions of Ubuntu can be installed using
PPA packages from the **named-data** repository.  First, you will need to add the
``named-data/ppa`` repository to the binary package sources and update the list of
available packages.

Preliminary steps if you have not used PPA packages before
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To simplify adding new PPA repositories, Ubuntu provides the ``add-apt-repository`` tool,
which is not installed by default on some systems.

::

    sudo apt install software-properties-common

Adding the NDN PPA
~~~~~~~~~~~~~~~~~~

After installing ``add-apt-repository``, run the following commands to add the `NDN PPA
repository`_::

    sudo add-apt-repository ppa:named-data/ppa
    sudo apt update

Installing NFD and other NDN packages
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

After you have added the `NDN PPA repository`_, NFD and other NDN packages can be easily
installed either using ``apt``, as shown below, or any other compatible package manager.

::

    sudo apt install nfd

For the list of available packages, refer to the `NDN PPA repository`_ page.

.. _NDN PPA repository: https://launchpad.net/~named-data/+archive/ppa

Building from source
--------------------

Downloading from git
~~~~~~~~~~~~~~~~~~~~

The first step is to obtain the source code for NFD and its main dependency, the
*ndn-cxx* library. If you do not want a development version of NFD, make sure you
checkout the correct release tag (e.g., ``*-0.7.1``) from both repositories.

.. code-block:: sh

    # Download ndn-cxx
    git clone https://github.com/named-data/ndn-cxx.git

    # Download NFD
    git clone --recursive https://github.com/named-data/NFD.git

.. note::
    While we strive to ensure that the latest version (git master branch) of NFD and ndn-cxx
    always compiles and works properly, we cannot guarantee that there will be no issues.
    If this is discovered to be the case, please use matching released versions (git tag or
    tarball) of NFD and ndn-cxx instead.

Prerequisites
~~~~~~~~~~~~~

Install the `ndn-cxx library <https://named-data.net/doc/ndn-cxx/current/INSTALL.html>`__
and its prerequisites.

On Linux, NFD needs the following dependencies to enable optional features:

- On **Ubuntu**:

  .. code-block:: sh

    sudo apt install libpcap-dev libsystemd-dev

- On **CentOS** and **Fedora**:

  .. code-block:: sh

    sudo dnf config-manager --enable powertools  # on CentOS only
    sudo dnf install libpcap-devel systemd-devel

Build
~~~~~

The following commands can be used to build and install NFD from source:

.. code-block:: sh

    ./waf configure  # on CentOS, add --without-pch
    ./waf
    sudo ./waf install

If you have installed ndn-cxx and/or any other dependencies into a non-standard path,
you may need to modify the ``PKG_CONFIG_PATH`` environment variable before running
``./waf configure``. For example:

.. code-block:: sh

    export PKG_CONFIG_PATH="/custom/lib/pkgconfig:$PKG_CONFIG_PATH"
    ./waf configure
    ./waf
    sudo ./waf install

Refer to ``./waf --help`` for more options that can be used during the ``configure`` stage.

.. note::
    If you are working on a source repository that has been compiled before, and you have
    upgraded one of the dependencies, please execute ``./waf distclean`` to clear object files
    and start over.

Debug symbols
~~~~~~~~~~~~~

The default compiler flags include debug symbols in binaries. This should provide
more meaningful debugging information if NFD or other tools happen to crash.

If this is not desired, the default flags can be overridden to disable debug symbols.
The following example shows how to completely disable debug symbols and configure
NFD to be installed into ``/usr`` with configuration in the ``/etc`` directory.

.. code-block:: sh

    CXXFLAGS="-O2" ./waf configure --prefix=/usr --sysconfdir=/etc
    ./waf
    sudo ./waf install

For Ubuntu PPA packages, debug symbols are available in ``*-dbg`` packages.

Customizing the compiler
~~~~~~~~~~~~~~~~~~~~~~~~

To build NFD with a different compiler (rather than the platform default), set the
``CXX`` environment variable to point to the compiler binary. For example, to build
with clang on Linux, use the following:

.. code-block:: sh

    CXX=clang++ ./waf configure

Building the documentation
~~~~~~~~~~~~~~~~~~~~~~~~~~

Tutorials and API documentation can be built using the following commands:

.. code-block:: sh

    # Full set of documentation (tutorials + API) in build/docs
    ./waf docs

    # Only tutorials in build/docs
    ./waf sphinx

    # Only API docs in build/docs/doxygen
    ./waf doxygen

If ``sphinx-build`` is detected during ``./waf configure``, manpages will automatically
be built and installed during the normal build process (i.e., during ``./waf`` and
``./waf install``). By default, manpages will be installed into ``${PREFIX}/share/man``
(the default value for ``PREFIX`` is ``/usr/local``). This location can be changed
during the ``./waf configure`` stage using the ``--prefix``, ``--datarootdir``, or
``--mandir`` options.

For further details, please refer to ``./waf --help``.

Initial configuration
---------------------

.. note::
    If you have installed NFD from binary packages, the package manager has already
    installed a working configuration and you can safely skip this section.

General
~~~~~~~

After installing NFD from source, you need to create a proper configuration file.
If the default installation directories were used with ``./waf configure``, this
can be accomplished by simply copying the sample configuration file as follows::

    sudo cp /usr/local/etc/ndn/nfd.conf.sample /usr/local/etc/ndn/nfd.conf

NFD Security
~~~~~~~~~~~~

NFD provides mechanisms to enable strict authorization for all management commands. In
particular, one can authorize only specific public keys to create new faces or change the
forwarding strategy for specific namespaces. For more information about how to generate
public/private key pairs, generate self-signed certificates, and use them to authorize
NFD management commands, refer to the :ref:`How do I configure NFD security` FAQ question.

In the sample configuration file, all security mechanisms are disabled for local clients,
effectively allowing anybody on the local machine to issue NFD management commands.

.. note::
    The sample configuration file is intended only for demo purposes and should NOT be
    used in production environments.

Running
-------

Starting
~~~~~~~~

If you have installed NFD from source, it is recommended to start NFD with the
``nfd-start`` script::

    nfd-start

On macOS, this command may ask for your keychain password or ask "nfd wants to sign using
key [xyz] in your keychain". Enter your keychain password and click "Always Allow".

Later, you can stop NFD with ``nfd-stop`` or by simply killing the ``nfd`` process.

If you have installed NFD using a package manager, you can start and stop NFD using the
operating system's service manager, such as ``systemctl`` or ``launchctl``.

Connecting to remote forwarders
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To create a UDP tunnel to a remote instance of NFD, execute the following command
in a terminal::

    nfdc face create udp://<other-host>

where ``<other-host>`` is the name or IP address of the other host (e.g.,
``udp://ndn.example.net``). If successful, this will print something like::

    face-created id=308 local=udp4://10.0.2.15:6363 remote=udp4://131.179.196.46:6363 persistency=persistent

To add a route ``/ndn`` toward this remote forwarder, execute the following command
in a terminal::

    nfdc route add /ndn udp://<other-host>

This will print::

    route-add-accepted prefix=/ndn nexthop=308 origin=static cost=0 flags=child-inherit expires=never

This indicates that NFD will forward all Interests that start with ``/ndn`` through the
face to the other host.  This forwards Interests to the other host, but does not provide
a "back route" for the other host to forward Interests to you.  For this, you can rely on
the "automatic prefix propagation" feature of NFD or use the ``nfdc`` command on the other
host to add the route.

Playing with NFD
----------------

After you have installed, configured, and started NFD, you can demonstrate the features
of NDN using the following applications and libraries.

Sample applications:

    + `Simple examples using the ndn-cxx library <https://named-data.net/doc/ndn-cxx/current/examples.html>`__
    + `Introductory examples of NDN-CCL
      <https://redmine.named-data.net/projects/application-development-documentation-guides/wiki/Step-By-Step_-_Common_Client_Libraries>`__

Real applications and libraries:

    + `ndn-tools - Essential NDN command-line tools <https://github.com/named-data/ndn-tools>`__
    + `ndn-traffic-generator - Traffic generator for NDN <https://github.com/named-data/ndn-traffic-generator>`__
    + `ChronoSync - Sync library for multi-user real-time applications <https://github.com/named-data/ChronoSync>`__
    + `PSync - Partial and full synchronization library <https://github.com/named-data/PSync>`__
