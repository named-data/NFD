Getting Started with NFD
========================

A more recent version of this document may be available on `NFD Wiki
<http://redmine.named-data.net/projects/nfd/wiki/Getting_Started>`_.

.. toctree::
   :maxdepth: 2

   INSTALL

Installing
----------

From source
~~~~~~~~~~~

To install NFD from source:

-  install ``ndn-cxx`` according to `ndn-cxx installation
   instructions <http://named-data.net/doc/ndn-cxx/0.1.0/INSTALL.html>`__
-  install ``NFD`` according to :doc:`these
   instructions <INSTALL>`

If downloading code using git, don't forget to checkout the correct
branch for both ``ndn-cxx`` and ``NFD``

::

    git clone https://github.com/named-data/ndn-cxx
    cd ndn-cxx
    git checkout ndn-cxx-0.1.0
    ...
    # continue with ndn-cxx installation instructions

    git clone https://github.com/named-data/NFD
    cd NFD
    git checkout NFD-0.1.0
    ...
    # continue with NFD installation instructions

From binaries
~~~~~~~~~~~~~

On OSX, NFD can be installed with MacPorts.  Refer to :ref:`How to start using NDN
MacPorts repository on OSX` FAQ question for more details.

On Ubuntu 12.04, 13.10, or 14.04, NFD can be installed from NDN PPA repository.  Refer to
:ref:`How to start using NDN PPA repository on Ubuntu Linux` FAQ question.


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

**You should not run ndnd or ndnd-tlv, otherwise NFD will not work
correctly**

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

To create a UDP or TCP tunnel to remote NFD and create route toward it,
use the following command in terminal:

::

    nfdc register /ndn udp://<other host>

where ``<other host>`` is the name or IP address of the other host
(e.g., ``udp://spurs.cs.ucla.edu``). This outputs:

::

    Successful in name registration: ControlParameters(Name: /ndn, FaceId: 10, Origin: 0, Cost: 0, Flags: 1, ExpirationPeriod: 3600000 milliseconds, )

The ``/ndn`` means that NFD will forward all Interests that start with ``/ndn`` through
the face to the other host.  If you only want to forward Interests with a certain prefix,
use it instead of ``/ndn``.  This only forwards Interests to the other host, but there is
no "back route" for the other host to forward Interests to you.  For that, you must go to
the other host and use ``nfdc`` to add the route.

The "back route" can also be automatically configured with ``nfd-autoreg``. For more
information refer to `nfd-autoreg manpage
<http://named-data.net/doc/NFD/current/manpages/nfd-autoreg.html>`_.

Playing with NFD
----------------

After you installed, configured, and started NFD, you can try to install
and play with the following:

Sample applications:

-  `Simple examples in ndn-cxx
   library <http://named-data.net/doc/ndn-cxx/0.1.0/examples.html>`__.
   If you have installed ndn-cxx from source, you already have compiled
   these:

   +  examples/producer
   +  examples/consumer
   +  examples/consumer-with-timer

   +  tools/ndncatchunks3
   +  tools/ndnputchunks3

-  `Introductory examples of
   NDN-CCL <http://redmine.named-data.net/projects/nfd/wiki/Getting_Started_-_Common_Client_Libraries#Install-the-Common-Client-Library>`__

Real applications and libraries:

   +  `ndn-tlv-ping - Ping Application For
      NDN <https://github.com/named-data/ndn-tlv-ping>`__
   +  `ndn-traffic-generator - Traffic Generator For
      NDN <https://github.com/named-data/ndn-traffic-generator>`__
   +  `repo-ng - Next generation of NDN
      repository <https://github.com/named-data/repo-ng>`__
   +  `ChronoChat - Multi-user NDN chat
      application <https://github.com/named-data/ChronoChat>`__
   +  `ChronoSync - Sync library for multiuser realtime applications for
      NDN <https://github.com/named-data/ChronoSync>`__
