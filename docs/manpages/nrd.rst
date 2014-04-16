NRD (NFD RIB Daemon)
========================

NRD is a complementary daemon designed to work in parallel with NFD
and to provide services for Routing Information Base management and
constructing NFD's FIB:

* prefix registration from applications;
* manual prefix registration;
* prefix registrations from multiple dynamic routing protocols.

Running and Configuring NRD
---------------------------

NRD's runtime settings may be modified via configuration file. After
installation, a working sample configuration is provided at
``SYSCONFDIR/ndn/nrd.conf.sample``. At startup, NRD will attempt to read
the default configuration file from following location:
``SYSCONFDIR/ndn/nrd.conf``.

You may also specify an alternative configuration file location by
running NRD with:

::

    nrd --config </path/to/nrd.conf>

Currently, nrd.conf contains only a security section, which is well
documented in the sample configuration file.

Starting experiments
--------------------

1. Build, install and setup NFD by following the directions provided in
   README.md in NFD root folder.

2. Create certificates by following the instructions in NFD's README.md

3. Build and install NRD by following the directions in INSTALL.md

4. Create nrd.conf file. You can simply copy the provided sample file in
   ``SYSCONFDIR/ndn/nrd.conf``.

5. Setup the trust-anchors in nrd.conf. Ideally, the prefix registration
   Interest should be signed by application/user key, and this
   application/user key should be used as trust-anchor. However,
   currently the default certificate is used to sign the registration
   Interests. Therefore, for initial testing the default certificate
   should be set as the trust-anchor. For doing so, the default
   certificate should be copied in the same folder where the nrd.conf
   is. Secondly, the trust-anchor should point to the copied certificate
   in nrd.conf. To know the id of default certificate you can use:

   ~$ ndnsec-get-default

6. Start NFD (assuming the NFD's conf file exist in
   ``SYSCONFDIR/ndn/nfd.conf``):

   ~$ nfd

7. Start NRD (assuming the conf file exist in
   ``SYSCONFDIR/ndn/nrd.conf``):

   ~$ nrd

8. Try to connect any application to NFD. For example, producer
   application that is provided ndn-cpp-dev examples.
