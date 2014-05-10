FAQ
===

How to change default paths?
----------------------------

Paths to where NFD is installed can be configured during ``./waf
configure``:

- Installation prefix (default ``/usr/local``):

    ::

        ./waf configure --prefix=/usr

- Location of NFD configuration file (default: ``${prefix}/etc``):

    ::

        ./waf configure --prefix=/usr --sysconfdir=/etc

- Location of manpages (default: ``${prefix}/share/man``)

    ::

        ./waf configure --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man

How to run NFD on Raspberry Pi?
-------------------------------

To run NFD on the Raspberry Pi, you need to either enable IPv6 support
in Raspberry Pi or disable IPv6 support in NFD.

To enable IPv6 in Raspberry Pi:

::

    sudo modprobe ipv6

To disable IPv6 in NFD, replace ``enable_v6 yes`` with ``enable_v6 no``
in ``tcp`` and ``udp`` sections of ``/usr/local/etc/ndn/nfd.conf``:

::

    ...
    tcp
    {
      listen yes
      port 6363
      enable_v4 yes
      enable_v6 no
    }

    udp
    {
      port 6363
      enable_v4 yes
      enable_v6 no
      idle_timeout 600
      keep_alive_interval 25

      mcast yes
      mcast_port 56363
      mcast_group 224.0.23.170
    }
    ...


How to run NFD as non-root user?
--------------------------------

How to configure automatic dropping of privileges?
++++++++++++++++++++++++++++++++++++++++++++++++++

NFD can be configured to drop privileges whenever possible.  You can specify a user and/or
group for NFD to change its *effective* user/group ID to in the ``general`` section of the
configuration file. For example:

::

    general
    {
      user nobody
      group nogroup
    }

will configure NFD to drop its effective user and group IDs to ``nobody`` and ``nogroup``,
respectively.

.. note::

    **IMPORTANT:** NFD may regain elevated permissions as needed during normal
    execution. Dropping privileges in this manner should not be considered a security
    mechanism (a compromised NFD that was started as root can trivially return to
    root). However, reducing privileges may limit any damaged caused by well intentioned,
    but buggy, code.


How to enable Ethernet Face Support?
++++++++++++++++++++++++++++++++++++

The ``ether`` configuration file section contains settings for Ethernet faces and
channels. These settings will **NOT** work without root or setting the appropriate
permissions:

::

    sudo setcap cap_net_raw,cap_net_admin=eip /full/path/nfd

You may need to install a package to use setcap:

**Ubuntu:**

::

    sudo apt-get install libcap2-bin

**Mac OS X:**

::

    curl https://bugs.wireshark.org/bugzilla/attachment.cgi?id=3373 -o ChmodBPF.tar.gz
    tar zxvf ChmodBPF.tar.gz
    open ChmodBPF/Install\ ChmodBPF.app

or manually:

::

    sudo chgrp admin /dev/bpf*
    sudo chmod g+rw /dev/bpf*

How to enable UDP multicast support in multi-homed Linux machines
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

The UDP configuration file section contains settings for unicast and multicast UDP
faces. If the Linux box is equipped with multiple network interfaces with multicast
capabilities, the settings for multicast faces will **NOT** work without root
or setting the appropriate permissions:

::

    sudo setcap cap_net_raw=eip /full/path/nfd

.. _How to configure NFD security:

How to configure NFD security?
------------------------------

.. note:: Sample configuration file of NFD allow any user to manage faces, FIB, RIB, and
    StrategyChoice.  The following description can be used to restrict certain operations
    to certain users.

    More extensive documentation about NFD's security and options to configure trust model
    for NFD is currently in preparation.

Many NFD management protocols require signed commands to be processed
(e.g. FIB modification, Face creation/destructions, etc.). You will need
an NDN certificate to use any application that issues signed commands.

If you do not already have NDN certificate, you can generate one with
the following commands:

**Generate and install a self-signed identity certificate**:

::

    ndnsec-keygen /`whoami` | ndnsec-install-cert -

Note that the argument to ndnsec-key will be the identity name of the
new key (in this case, ``/your-username``). Identity names are
hierarchical NDN names and may have multiple components (e.g.
``/ndn/ucla/edu/alice``). You may create additional keys and identities
as you see fit.

**Dump the NDN certificate to a file**:

The following commands assume that you have not modified ``PREFIX`` or
``SYSCONFDIR`` If you have, please substitute ``/usr/local/etc`` for the
appropriate value (the overriden ``SYSCONFDIR`` or ``PREFIX/etc`` if you
changed ``PREFIX``).

::

    sudo mkdir -p /usr/local/etc/ndn/keys
    ndnsec-cert-dump -i /`whoami` > default.ndncert
    sudo mv default.ndncert /usr/local/etc/ndn/keys/default.ndncert

.. _How to start using NDN MacPorts repository on OSX:

How to start using NDN MacPorts repository on OSX?
--------------------------------------------------

OSX users have an opportunity to seamlessly install and run NFD and other related
applications via `MacPorts <https://www.macports.org/>`_. If you are not using MacPorts
yet, go to `MacPorts website <https://www.macports.org/install.php>`_ and download and
install the MacPorts package.

NFD and related ports are not part of the official MacPorts repository and in order to use
it, you need to add NDN MacPorts repository to the local configuration.  In particular,
you will need to modify the list of source URLs for MacPorts.  For example, if your
MacPorts are installed in ``/opt/local``, add the following line
`/opt/local/etc/macports/sources.conf` before or after the default port repository:

::

    rsync://macports.named-data.net/macports/

After this step, you can use ``sudo port selfupdate`` to fetch updated port definitions.

The following command will install NFD using MacPorts:

::

    sudo port install nfd

.. note::
    You have to have XCode installed on your machine.  For latest versions of OSX (Lion or
    Mountain Lion) you can install it from AppStore for free, for older versions you have to
    go to developer.apple.com and download old version of XCode that is appropriate for your
    system.


One of the advantages of using MacPorts is that you can easily upgrade NFD and other
packages to the most recent version available.  The following commands will do this job:

::

    sudo port selfupdate
    sudo port upgrade nfd

.. _How to start using NDN PPA repository on Ubuntu Linux:

How to start using NDN PPA repository on Ubuntu Linux?
------------------------------------------------------

NFD binaries and related tools for Ubuntu 12.04, 13.10, and 14.04 can be installed using
PPA packages from named-data repository.  First, you will need to add ``named-data/ppa``
repository to binary package sources and update list of available packages.

Preliminary steps if you haven't used PPA packages before
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

To simplify adding new PPA repositories, Ubuntu provides ``add-apt-repository`` tool, which
is not installed by default on some platforms.

On Ubuntu **12.04**:

::

    sudo apt-get install python-software-properties

On Ubuntu **13.10** and **14.04**:

::

    sudo apt-get install software-properties-common


Adding NDN PPA
++++++++++++++

After installing ``add-apt-repository``, run the following command to add `NDN PPA repository`_.

::

    sudo add-apt-repository ppa:named-data/ppa
    sudo apt-get update

Installing NFD and other NDN packages
+++++++++++++++++++++++++++++++++++++

After you have added `NDN PPA repository`_, NFD and other NDN packages can be easily
installed in a standard way, i.e., either using ``apt-get`` as shown below or using any
other package manager, such as Synaptic Package Manager:

::

    sudo apt-get install nfd

For the list of available packages, refer to `NDN PPA repository`_ homepage.

.. _NDN PPA repository: https://launchpad.net/~named-data/+archive/ppa
