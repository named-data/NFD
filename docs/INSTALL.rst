Getting Started with NFD
========================

Installing NFD from Binaries
----------------------------

We provide NFD binaries for the supported platforms, which are the preferred installation
method. In addition to simplifying installation, the binary release also includes
automatic initial configuration and platform-specific tools to automatically start NFD and
related daemons.  In particular, on OS X NFD is controlled using `launchd
<https://github.com/named-data/NFD/tree/master/contrib/osx-launchd>`__ and on Ubuntu using
`upstart <https://github.com/named-data/NFD/tree/master/contrib/upstart>`__ mechanisms.
In both cases, `nfd-start` and `nfd-stop` scripts are convenience wrappers for launchd and
upstart.

On OS X, NFD can be installed with MacPorts.  Refer to :ref:`Install NFD Using the NDN
MacPorts Repository on OS X` for more details.

On Ubuntu 12.04, 14.04, or 14.10 NFD can be installed from NDN PPA repository.  Refer to
:ref:`Install NFD Using the NDN PPA Repository on Ubuntu Linux`.

Future releases could include support for other platforms.  Please send us feedback on the
platforms you're using, so we can prioritize our goals.  We would also appreciate help
packaging the current NFD release for other platforms.


.. _Install NFD Using the NDN MacPorts Repository on OS X:

Install NFD Using the NDN MacPorts Repository on OS X
-----------------------------------------------------

OS X users have the opportunity to seamlessly install and run NFD as well as other related
applications via `MacPorts <https://www.macports.org/>`_.  If you are not using MacPorts
yet, go to the `MacPorts website <https://www.macports.org/install.php>`_ and install the
MacPorts package.

NFD and related ports are not part of the official MacPorts repository. In order to use
these ports, you will need to add the NDN MacPorts repository to your local configuration.
In particular, you will need to modify the list of source URLs for MacPorts.  For example,
if your MacPorts are installed in ``/opt/local``, add the following line to
`/opt/local/etc/macports/sources.conf` before or after the default port repository:

::

    rsync://macports.named-data.net/macports/

After this step, you can use ``sudo port selfupdate`` to fetch updated port definitions.

The following command will install NFD using MacPorts:

::

    sudo port install nfd

.. note::

    You have to have XCode installed on your machine. This can be installed from the
    AppStore (free) on OS X 10.7 or later. Older editions of OS X can download an
    appropriate version of XCode from http://developer.apple.com.


One advantage of using MacPorts is that you can easily upgrade NFD and other packages to
the latest version.  The following commands will do this job:

::

    sudo port selfupdate
    sudo port upgrade nfd

.. _Install NFD Using the NDN PPA Repository on Ubuntu Linux:

Install NFD Using the NDN PPA Repository on Ubuntu Linux
--------------------------------------------------------

NFD binaries and related tools for Ubuntu 12.04, 14.04, or 14.10 can be installed using PPA
packages from named-data repository.  First, you will need to add ``named-data/ppa``
repository to binary package sources and update list of available packages.

Preliminary steps if you haven't used PPA packages before
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To simplify adding new PPA repositories, Ubuntu provides ``add-apt-repository`` tool,
which is not installed by default on some platforms.

On Ubuntu **12.04**:

::

    sudo apt-get install python-software-properties

On Ubuntu **14.04** or **14.10**:

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
sure you checkout the correct release tag (e.g., ``*-0.2.0``) for both repositories:

::

    # Download ndn-cxx
    git clone https://github.com/named-data/ndn-cxx

    # Download NFD
    git clone --recursive https://github.com/named-data/NFD

Prerequisites
~~~~~~~~~~~~~

-  Install the `ndn-cxx library <http://named-data.net/doc/ndn-cxx/current/INSTALL.html>`_
   and its requirements

-  ``pkg-config``

   On OS X 10.8, 10.9, and 10.10 with MacPorts:

   ::

       sudo port install pkgconfig

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install pkg-config

-  ``libpcap``

   Comes with the base system on OS X 10.8, 10.9, and 10.10.

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install libpcap-dev

To build manpages and API documentation:

-  ``doxygen``
-  ``graphviz``
-  ``python-sphinx``

   On OS X 10.8, 10.9, and 10.10 with MacPorts:

   ::

       sudo port install doxygen graphviz py27-sphinx sphinx_select
       sudo port select sphinx py27-sphinx

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install doxygen graphviz python-sphinx


Besides officially supported platforms, NFD is known to work on: Fedora 20, CentOS 6/7, Gentoo Linux,
Raspberry Pi, OpenWRT, FreeBSD 10.0, and several other platforms.  We are soliciting help
with documenting common problems / pitfalls in installing/using NFD on different platforms
on `NFD Wiki
<http://redmine.named-data.net/projects/nfd/wiki/Wiki#Installation-experiences-for-selected-platforms>`__.


Build
~~~~~

The following basic commands should be used to build NFD on Ubuntu:

::

    ./waf configure
    ./waf
    sudo ./waf install

If you have installed `ndn-cxx` library and/or other dependencies into a non-standard paths, you
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

Customize Compiler
~~~~~~~~~~~~~~~~~~

To customize compiler, set ``CXX`` environment variable to point to compiler binary and, in
some case, specify type of the compiler using ``--check-cxx-compiler``.  For example, when
using clang compiler on Linux system, use the following:

::

    CXX=clang++ ./waf configure --check-cxx-compiler=clang++

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
certificate to authorize NFD management commands refer to :ref:`How to configure NFD
security` FAQ question.

In the sample configuration file, all authorizations are disabled, effectively allowing
anybody on the local machine to issue NFD management commands. **The sample file is
intended only for demo purposes and MUST NOT be used in a production environment.**

Running
-------

**You should not run ndnd or ndnd-tlv, otherwise NFD will not work correctly**

Starting
~~~~~~~~

In order to use NFD, you need to start two separate daemons: ``nfd`` (the forwarder
itself) and ``nrd`` (RIB manager that will manage all prefix registrations).  The
recommended way is to use `nfd-start` script:

::

    nfd-start

On OS X it may ask for your keychain password or ask ``nfd/nrd wants to sign using key in
your keychain.`` Enter your keychain password and click Always Allow.

Later, you can stop NFD with ``nfd-stop`` or by simply killing the ``nfd`` process.


Connecting to remote NFDs
~~~~~~~~~~~~~~~~~~~~~~~~~

To create a UDP or TCP tunnel to remote NFD and create route toward it, use the following
command in terminal:

::

    nfdc register /ndn udp://<other host>

where ``<other host>`` is the name or IP address of the other host (e.g.,
``udp://spurs.cs.ucla.edu``). This outputs:

::

    Successful in name registration: ControlParameters(Name: /ndn, FaceId: 260, Origin: 255, Cost: 0, Flags: 1, )

The ``/ndn`` means that NFD will forward all Interests that start with ``/ndn`` through
the face to the other host.  If you only want to forward Interests with a certain prefix,
use it instead of ``/ndn``.  This only forwards Interests to the other host, but there is
no "back route" for the other host to forward Interests to you.  For that, you must go to
the other host and use ``nfdc`` to add the route.

The "back route" can also be automatically configured with ``nfd-autoreg``. For more
information refer to :doc:`manpages/nfd-autoreg`.

Playing with NFD
----------------

After you haved installed, configured, and started NFD, you can try to install and play
with the following:

Sample applications:

- `Simple examples in ndn-cxx library <http://named-data.net/doc/ndn-cxx/current/examples.html>`_

   If you have installed ndn-cxx from source, you already have compiled these:

   +  examples/producer
   +  examples/consumer
   +  examples/consumer-with-timer

   +  tools/ndncatchunks3
   +  tools/ndnputchunks3

- `Introductory examples of NDN-CCL
  <http://redmine.named-data.net/projects/application-development-documentation-guides/wiki/Step-By-Step_-_Common_Client_Libraries>`_

Real applications and libraries:

   + `ndn-tlv-ping - Reachability Testing Tool for NDN
     <https://github.com/named-data/ndn-tlv-ping>`_
   + `ndn-traffic-generator - Traffic Generator For NDN
     <https://github.com/named-data/ndn-traffic-generator>`_
   + `repo-ng - Next generation of NDN repository <https://github.com/named-data/repo-ng>`_
   + `ChronoChat - Multi-user NDN chat application <https://github.com/named-data/ChronoChat>`_
   + `ChronoSync - Sync library for multiuser realtime applications for NDN
     <https://github.com/named-data/ChronoSync>`_
