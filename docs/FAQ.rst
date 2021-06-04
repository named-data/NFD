FAQ
===

How do I change the default installation paths?
-----------------------------------------------

Paths to where NFD is installed can be configured during ``./waf configure``:

- Installation prefix (default ``/usr/local``)::

    ./waf configure --prefix=/usr

- Location of NFD configuration file (default: ``${prefix}/etc``)::

    ./waf configure --prefix=/usr --sysconfdir=/etc

- Location of manpages (default: ``${prefix}/share/man``)::

    ./waf configure --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man

See ``./waf configure --help`` for the full list of options.

How do I use the NDN PPA repository on Ubuntu Linux?
----------------------------------------------------

Please see :ref:`Install NFD on Ubuntu Linux using the NDN PPA repository`.

How do I run NFD as a non-root user?
------------------------------------

How do I configure automatic privilege dropping?
++++++++++++++++++++++++++++++++++++++++++++++++

NFD can be configured to drop privileges whenever possible.  You can specify a user and/or
group for NFD to change its *effective* user/group ID to in the ``general`` section of the
configuration file. For example::

    general
    {
      user nobody
      group nogroup
    }

will configure NFD to drop its effective user and group IDs to ``nobody`` and ``nogroup``,
respectively.

.. note::

    **IMPORTANT:** NFD may regain elevated privileges as needed during normal
    execution. Dropping privileges in this manner should not be considered a security
    mechanism (a compromised NFD that was started as root can trivially return to
    root). However, reducing privileges may limit any damage caused by well intentioned,
    but buggy, code.

How do I enable Ethernet face support?
++++++++++++++++++++++++++++++++++++++

The ``ether`` configuration file section contains settings for Ethernet faces and
channels. These settings will **NOT** work without root or without setting the
appropriate permissions.

- On **Ubuntu**::

    sudo apt install libcap2-bin
    sudo setcap cap_net_raw,cap_net_admin=eip /path/to/nfd

- On **macOS**::

    curl https://bugs.wireshark.org/bugzilla/attachment.cgi?id=3373 -o ChmodBPF.tar.gz
    tar zxvf ChmodBPF.tar.gz
    open ChmodBPF/Install\ ChmodBPF.app

  or manually::

    sudo chgrp admin /dev/bpf*
    sudo chmod g+rw /dev/bpf*

How do I enable UDP multicast support in multi-homed Linux machines?
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

The ``udp`` configuration file section contains settings for unicast and multicast UDP
faces. If the Linux machine is equipped with multiple network interfaces with multicast
capabilities, the settings for multicast faces will **NOT** work without root or without
setting the appropriate permissions::

    sudo setcap cap_net_raw=eip /path/to/nfd

.. _How do I configure NFD security:

How do I configure NFD security?
--------------------------------

.. note:: The sample configuration file for NFD allows any user to manage faces, FIB, RIB,
    CS, and strategy choices of the local NFD instance. The following procedure can be used
    to restrict certain operations to certain users.

    More extensive documentation on the security mechanisms in NFD, as well as the available
    options to configure its trust model, is currently in preparation.

Many management components in NFD use *Command Interests* (e.g., FIB modification, face
creation/destruction, etc.), which require an NDN certificate (either self-signed for local
trust or delegated from a trusted authority).

If you do not already have an NDN certificate, you can generate one using the following procedure.

**Generating and installing a self-signed identity certificate**:

::

    ndnsec key-gen /$(whoami) | ndnsec cert-install -

Note that the argument to ``ndnsec key-gen`` will be the identity name of the new key (in this
case, ``/your-username``). Identity names are hierarchical NDN names and may have multiple
components (e.g.  ``/ndn/ucla/edu/alice``). You may create additional keys and identities as
needed.

**Exporting the NDN certificate to a file**:

The following commands assume that you have not modified ``PREFIX`` or ``SYSCONFDIR``.
If you have, please substitute the appropriate path in place of ``/usr/local/etc``.

::

    sudo mkdir -p /usr/local/etc/ndn/keys
    ndnsec cert-dump -i /$(whoami) > default.ndncert
    sudo mv default.ndncert /usr/local/etc/ndn/keys/default.ndncert
