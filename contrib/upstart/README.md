Starting NFD on Linux with upstart
==================================

Some Linux distributions, such as Ubuntu, use [upstart](http://upstart.ubuntu.com/) as a
standard mechanism to start system daemons, monitor their health, and restart
when they die.

Initial setup
-------------

* Edit `nfd.conf` correcting paths for `nfd` binary, configuration and log files.

* Copy upstart config file for NFD

        sudo cp nfd.conf /etc/init/

### Assumptions in the default scripts

* `nfd` is installed into `/usr/local/bin`
* Configuration file is `/usr/local/etc/ndn/nfd.conf`
* `nfd` will be run as root
* Log files will be written to `/usr/local/var/log/ndn` folder, which is owned by user `ndn`

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

`HOME` directory for `nfd` should be created prior to starting.  This is necessary to manage
unique security credentials for the deamon.

    # Create HOME and generate self-signed NDN certificate for nfd
    sudo -s -- ' \
      mkdir -p /usr/local/var/lib/ndn/nfd/.ndn; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      ndnsec-keygen /localhost/daemons/nfd | ndnsec-install-cert -; \
    '

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

While this configuration still allows management of faces and updating strategy choice by
anybody, only NFD's RIB Manager (i.e., NFD itself) is allowed to manage FIB.

As the final step to make this configuration work, nfd's self-signed certificate needs to
be exported into `localhost_daemons_nfd.ndncert` file:

    sudo -s -- '\
      mkdir -p /usr/local/etc/ndn/certs || true; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      ndnsec-dump-certificate -i /localhost/daemons/nfd > \
        /usr/local/etc/ndn/certs/localhost_daemons_nfd.ndncert; \
      '


Enable auto-start
-----------------

After copying the provided upstart script, `nfd` daemon will automatically run after the reboot.
To manually start them, use the following commands:

    sudo start nfd

Disable auto-start
------------------

To stop `nfd` daemon, use the following commands:

    sudo stop nfd

Note that as long as upstart files are present in `/etc/init/`, the daemon will
automatically start after the reboot.  To permanently stop `nfd` daemon, delete
the upstart files:

    sudo rm /etc/init/nfd.conf
