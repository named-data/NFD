Starting NFD on Linux with upstart
==================================

Some Linux distributions, such as Ubuntu, use [upstart](http://upstart.ubuntu.com/) as a
standard mechanism to start system daemons, monitor their health, and restart
when they die.

Initial setup
-------------

Edit `nfd.conf` and `nrd.conf` correcting paths for `nfd` and `nfd` binaries,
configuration file, and log files.

    # Copy upstart config file for nfd (forwarding daemon)
    sudo cp nfd.conf /etc/init/

    # Copy upstart config file for nrd (RIB management daemon)
    sudo cp nrd.conf /etc/init/

    # Copy upstart config file for nfd-watcher (will restart NFD when network change detected)
    sudo cp nfd-watcher.conf /etc/init/

### Assumptions in the default scripts

* `nfd` and `nrd` are installed into `/usr/local/bin`
* Configuration file is `/usr/local/etc/ndn/nfd.conf`
* `nfd` will be run as root
* `nrd` will be run as user `ndn` and group `ndn`
* Log files will be written to `/usr/local/var/log/ndn` folder, which is owned by user `ndn`
* Whenever network connectivity changes, both `nfd` and `nrd` are restarted

### Creating users

If `ndn` user and group does not exists, they need to be manually created.

    # Create group `ndn`
    addgroup --system ndn

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

`HOME` directories for `nfd` and `nrd` should be created prior to starting.  This is
necessary to manage unique security credentials for the deamons.

    # Create HOME and generate self-signed NDN certificate for nfd
    sudo mkdir -p /usr/local/var/lib/ndn/nfd/.ndn
    sudo HOME=/usr/local/var/lib/ndn/nfd ndnsec-keygen /localhost/daemons/nfd | \
      sudo HOME=/usr/local/var/lib/ndn/nfd ndnsec-install-cert -

    # Create HOME and generate self-signed NDN certificate for nrd
    sudo mkdir -p /usr/local/var/lib/ndn/nrd/.ndn
    sudo chown -R ndn:ndn /usr/local/var/lib/ndn/nrd
    sudo -u ndn -g ndn HOME=/usr/local/var/lib/ndn/nrd ndnsec-keygen /localhost/daemons/nrd | \
      sudo -u ndn -g ndn HOME=/usr/local/var/lib/ndn/nrd ndnsec-install-cert -

### Configuring NFD's security

NFD sample configuration allows anybody to create faces, add nexthops to FIB, and set
strategy choice for namespaces.  While such settings could be a good start, it is
generally not a good idea to run NFD in this mode.

While thorough discussion about security configuration of NFD is outside the scope of this
document, at least the following change should be done to ``nfd.conf`` in authorize
section:

    authorizations
    {
      authorize
      {
        certfile certs/localhost_daemons_nrd.ndncert
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

While this configuration still allows management of faces and updating strategy choice by
anybody, only NFD's RIB Manager Daemon (`nrd`) is allowed to manage FIB.

As the final step to make this configuration work, nrd's self-signed certificate needs to
be exported into `localhost_daemons_nrd.ndncert` file:

    sudo mkdir /usr/local/etc/ndn/certs
    sudo sh -c 'sudo -u ndn -g ndn HOME=/usr/local/var/lib/ndn/nrd \
      ndnsec-dump-certificate -i /localhost/daemons/nrd \
      > /usr/local/etc/ndn/certs/localhost_daemons_nrd.ndncert'


Enable auto-start
-----------------

After copying the provided upstart scripts, `nfd` and `nrd` daemons will automatically run
after the reboot.  To manually start them, use the following commands:

    sudo start nfd
    # nrd will be automatically started by upstart

Note that an additional upstart job, ``nfd-watcher``, will automatically monitor for
network connectivity changes, such as when network interface gets connected, disconnected,
or IP addresses of the network interface get updated.  When ``nfd-watcher`` detects the
event, it will restart `nfd` and `nrd`.

Disable auto-start
------------------

To stop `nrd` and `nfd` daemon, use the following commands:

    sudo stop nfd
    # nrd will be automatically stopped by upstart

Note that as long as upstart files are present in `/etc/init/`, the daemons will
automatically start after the reboot.  To permanently stop `nfd` and `nrd` daemons, delete
the upstart files:

    sudo rm /etc/init/nfd.conf
    sudo rm /etc/init/nrd.conf
    sudo rm /etc/init/nfd-watcher.conf
