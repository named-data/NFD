.. _NFD Configuration Tips:

NFD - Named Data Networking Forwarding Daemon
=============================================

Default Paths
-------------

This document uses ``SYSCONFDIR`` when referring to the default locations
of various NFD configuration files. By default, ``SYSCONFDIR`` is set to
``/usr/local/etc``. If you override ``PREFIX``, then ``SYSCONFDIR`` will
default to ``PREFIX/etc``.

You may override ``SYSCONFDIR`` and ``PREFIX`` by specifying their
corresponding options during installation:

::

    ./waf configure --prefix <path/for/prefix> --sysconfdir <some/other/path>

Refer to :ref:`NFD Installation Instructions` for more detailed instructions on how to compile
and install NFD.

Running and Configuring NFD
---------------------------

NFD's runtime settings may be modified via configuration file. After
installation, a working sample configuration is provided at
``SYSCONFDIR/ndn/nfd.conf.sample``. At startup, NFD will attempt to read
the default configuration file location: ``SYSCONFDIR/ndn/nfd.conf``.

You may also specify an alternative configuration file location by
running NFD with:

::

    nfd --config </path/to/nfd.conf>

Once again, note that you may simply copy or rename the provided sample
configuration and have an **almost** fully configured NFD. However, this
NFD will be unable to add FIB entries or perform other typical operation
tasks until you authorize an NDN certificate with the appropriate
privileges.

Installing an NDN Certificate for Command Authentication
--------------------------------------------------------

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

Running NFD with Ethernet Face Support
--------------------------------------

The ether configuration file section contains settings for Ethernet
faces and channels. These settings will **NOT** work without root or
setting the appropriate permissions:

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

UDP multicast support in multi-homed Linux machines
---------------------------------------------------

The UDP configuration file section contains settings for unicast and multicast UDP
faces. If the Linux box is equipped with multiple network interfaces with multicast
capabilities, the settings for multicast faces will **NOT** work without root
or setting the appropriate permissions:

::

    sudo setcap cap_net_raw=eip /full/path/nfd
