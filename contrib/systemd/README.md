Starting NFD on Linux with systemd
==================================

Newer versions of Ubuntu (starting with 15.04) and some other Linux distributions, including Debian
use systemd to start system daemons, monitor their health, and restart them when they die.

Initial setup
-------------

* Edit `nfd.service`, correcting the paths to the `nfd` executable, configuration, and
  ``HOME`` directories.

* Copy the systemd config file for NFD to the proper directory

        sudo cp nfd.service /etc/systemd/system

* Reload the systemd manager configuration

        sudo systemctl daemon-reload

### Assumptions in the default scripts

* `nfd` is installed into `/usr/local/bin`
* Configuraton file is `/usr/local/etc/ndn/nfd.conf`
* `nfd` will be run as root
* Log files will be written to `/usr/local/var/log/ndn` folder, which is owned by user `ndn`

### Creating users

If the `ndn` user and group do not exist, they need to be manually created.

    # Create group `ndn`
    sudo addgroup --system ndn

    # Create user `ndn`
    sudo adduser --system \
                 --disabled-login \
                 --ingroup ndn \
                 --home /nonexistent \
                 --gecos "NDN User" \
                 --shell /bin/false \
                 ndn


### Creating folders

Folder `/usr/local/var/log/ndn` should be created and assigned proper user and group:

    sudo mkdir -p /usr/local/var/log/ndn
    sudo chown -R ndn:ndn /usr/local/var/log/ndn

`HOME` directory for `nfd` should be created prior to starting. This is necessary to manage
unique security credentials for the daemon.

    # Create HOME and generate self-signed NDN certificate for nfd
    sudo sh -c ' \
      mkdir -p /usr/local/var/lib/ndn/nfd/.ndn; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      ndnsec-keygen /localhost/daemons/nfd | ndnsec-install-cert -; \
    '

### Configuring NFD's security

NFD sample configuration allows anybody to create faces, add nexthops to FIB, and set
strategy choice for namespaces. While such settings could be a good start, it is
generally not a good idea to run NFD in this mode.

While thorough discussion about the security configuration of NFD is outside the scope of
this document, at least the following change should be done in ``nfd.conf`` in the
authorize section:

    authorizations
    {
      authorize
      {
        certfile certs/localhost_daemons_nfd.ndncert
        privileges
        {
            faces
            fib
            strategy-choice
        }
      }

      authorize
      {
        certfile any
        privileges
        {
            faces
            strategy-choice
        }
      }
    }

While this configuration still allows the management of faces and updating strategy choice by
anyone, only NFD's RIB Manager (i.e., NFD itself) is allowed to manage FIB.

As the final step to make this configuration work, nfd's self-signed certificate needs to
be exported into the `localhost_daemons_nfd.ndncert` file:

    sudo sh -c '\
      mkdir -p /usr/local/etc/ndn/certs || true; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      ndnsec-dump-certificate -i /localhost/daemons/nfd > \
        /usr/local/etc/ndn/certs/localhost_daemons_nfd.ndncert; \
    '

Enable auto-start
-----------------

After copying the provided upstart script, auto-start of the `nfd` daemon can be enabled with:

    sudo systemctl enable nfd

To manually start it, use the following command:

    sudo systemctl start nfd

Disable auto-start
------------------

To stop the `nfd` daemon, use the following command:

    sudo systemctl stop nfd

To permanently stop the `nfd` daemon and disable it from being automatically started on reboot,
disable the service:

    sudo systemctl disable nfd