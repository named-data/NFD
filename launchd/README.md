Starting NFD on OSX >= 10.8
===========================

OSX provides a standard way to start system daemons, monitor their health, and restart
when they die.

Initial setup
-------------

Edit `net.named-data.nfd` correcting paths for `nfd` binary, configuration and log files.

    # Copy launchd.plist for NFD
    sudo cp net.named-data.nfd.plist /Library/LaunchDaemons/
    sudo chown root /Library/LaunchDaemons/net.named-data.nfd.plist

### Assumptions in the default scripts

* `nfd` is installed into `/usr/local/bin`
* Configuration file is `/usr/local/etc/ndn/nfd.conf`
* `nfd` will be run as root
* Log files will be written to `/usr/local/var/log/ndn` folder, which is owned by user `ndn`

### Creating users

If `ndn` user does not exists, it needs to be manually created (procedure copied from
[macports script](https://trac.macports.org/browser/trunk/base/src/port1.0/portutil.tcl)).
Update uid/gid if 6363 is already used.

    # Create user `ndn`
    sudo dscl . -create /Users/ndn UniqueID 6363

    # These are implicitly added on Mac OSX Lion.  AuthenticationAuthority
    # causes the user to be visible in the Users & Groups Preference Pane,
    # and the others are just noise, so delete them.
    # https://trac.macports.org/ticket/30168
    sudo dscl . -delete /Users/ndn AuthenticationAuthority
    sudo dscl . -delete /Users/ndn PasswordPolicyOptions
    sudo dscl . -delete /Users/ndn dsAttrTypeNative:KerberosKeys
    sudo dscl . -delete /Users/ndn dsAttrTypeNative:ShadowHashData

    sudo dscl . -create /Users/ndn RealName "NDN User"
    sudo dscl . -create /Users/ndn Password "{*}"
    sudo dscl . -create /Users/ndn PrimaryGroupID 6363
    sudo dscl . -create /Users/ndn NFSHomeDirectory /var/empty
    sudo dscl . -create /Users/ndn UserShell /usr/bin/false

    # Create group `ndn`
    sudo dscl . -create /Groupsndn Password "{*}"
    sudo dscl . -create /Groups/ndn RealName "NDN User"
    sudo dscl . -create /Groups/ndn PrimaryGroupID 6363

### Creating folders

Folder `/usr/local/var/log/ndn` should be created and assigned proper user and group:

    sudo mkdir -p /usr/local/var/log/ndn
    sudo chown -R ndn:ndn /usr/local/var/log/ndn

`HOME` directory for `nfd` should be created and configured with correct library's config file
and contain proper NDN security credentials for signing Data packets.  This is necessary since
default private key storage on OSX (`osx-keychain`) does not support non-interactive access,
and file-based private key storage needs to be used:

    # Create HOME and generate self-signed NDN certificate for nfd
    sudo -s -- ' \
      mkdir -p /usr/local/var/lib/ndn/nfd/.ndn; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      echo tpm=tpm-file > /usr/local/var/lib/ndn/nfd/.ndn/client.conf; \
      ndnsec-keygen /localhost/daemons/nfd | ndnsec-install-cert -; \
    '

### Configuring NFD's security

NFD sample configuration allows anybody to create faces, add nexthops to FIB, and set strategy
choice for namespaces.  While such settings could be a good start, it is generally not a good
idea to run NFD in this mode.

While thorough discussion about security configuration of NFD is outside the scope of this
document, at least the following change should be done to nfd.conf in authorize section:

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

As the final step to make this configuration work, NFD's self-signed certificate needs to
be exported into `localhost_daemons_nfd.ndncert` file:

    sudo -s -- '\
      mkdir -p /usr/local/etc/ndn/certs || true; \
      export HOME=/usr/local/var/lib/ndn/nfd; \
      ndnsec-dump-certificate -i /localhost/daemons/nfd > \
        /usr/local/etc/ndn/certs/localhost_daemons_nfd.ndncert; \
      '


Enable auto-start
-----------------

    sudo launchctl load -w /Library/LaunchDaemons/net.named-data.nfd.plist

Disable auto-start
------------------

    sudo launchctl unload -w /Library/LaunchDaemons/net.named-data.nfd.plist
