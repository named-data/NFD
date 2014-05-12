NFD - Named Data Networking Forwarding Daemon
=============================================

NFD overview, release notes for the released versions, getting started tutorial, and other
additional documentation are available online on
[NFD's homepage](http://named-data.net/doc/NFD/).

## Overview

NFD is a network forwarder that implements and evolves together with the Named Data
Networking (NDN) [protocol](http://named-data.net/doc/ndn-tlv/).  After the initial
release, NFD will become a core component of the
[NDN Platform](http://named-data.net/codebase/platform/) and will follow the same release
cycle.

NFD is an open and free software package licensed under GPL 3.0 license and is the
centerpiece of our committement to making NDN's core technology open and free to all
Internet users and developers.  For more information about the licensing details and
limitation, refer to
[`COPYING.md`](https://github.com/named-data/NFD/blob/master/COPYING.md).

NFD is developed by a community effort.  Although the first release was mostly done by the
members of [NSF-sponsored NDN project team](http://named-data.net/project/participants/),
it already contains significant contributions from people outside the project team (for
more details, refer to [`AUTHORS.md`](https://github.com/named-data/NFD/blob/master/AUTHORS.md)).
We strongly encourage participation from all interested parties, since broader community
support is key for NDN to succeed as a new Internet architecture.  Bug reports and
feedback are highly appreciated and can be made through
[Redmine site](http://redmine.named-data.net/projects/nfd) and the
[ndn-interest mailing list](http://www.lists.cs.ucla.edu/mailman/listinfo/ndn-interest).

The main design goal of NFD is to support diverse experimentation of NDN technology.  The
design emphasizes *modularity* and *extensibility* to allow easy experiments with new
protocol features, algorithms, new applications.  We have not fully optimized the code for
performance.  The intention is that performance optimizations are one type of experiments
that developers can conduct by trying out different data structures and different
algorithms; over time, better implementations may emerge within the same design framework.

NFD will keep evolving in three aspects: improvement of the modularity framework, keeping
up with the NDN protocol spec, and addition of other new features. We hope to keep the
modular framework stable and lean, allowing researchers to implement and experiment
with various features, some of which may eventually work into the protocol spec.

The design and development of NFD benefited from our earlier experience with
[CCNx](http://www.ccnx.org) software package.  However, NFD is not in any part derived from
CCNx codebase and does not maintain compatibility with CCNx.

Binary release
--------------

As of release 0.1.0, we are providing NFD binaries for the supported platforms, which are
the preferred installation method of NFD:

- [Ubuntu 12.04, 13.10, and 14.04](http://named-data.net/doc/NFD/current/FAQ.html#how-to-start-using-ndn-ppa-repository-on-ubuntu-linux)

- [OSX 10.8, 10.9 with MacPorts](http://named-data.net/doc/NFD/current/FAQ.html#how-to-start-using-ndn-macports-repository-on-osx)

Next releases would include support for other platforms.  Please send us feedback on the
platforms you're using, so we can prioritize our goals.  We would also appreciate if
someone can provide help with packaging the current NFD release for other platforms.

Besides simplicity of installation, the binary release includes automatic initial
configuration and platform-specific tools to automatically start NFD and related daemons.
In particular, on OSX NFD is controlled using
[launchd](https://github.com/named-data/NFD/tree/master/contrib/osx-launchd) and on Ubuntu
using [upstart](https://github.com/named-data/NFD/tree/master/contrib/upstart) mechanisms.
In both cases, `nfd-start` and `nfd-stop` scripts are convenience wrappers for launchd and
upstart.

Source releases
---------------

The source code and source-code installation instructions are always available on NFD's homepage:

- [Getting Started with NFD](http://named-data.net/doc/NFD/current/getting-started.html)
- [Github NFD repository](https://github.com/named-data/NFD)

Additional information
----------------------

- [NFD Wiki](http://redmine.named-data.net/projects/nfd/wiki/)
- Feature requests and bug reports are welcome on our
  [NDN Redmine](http://redmine.named-data.net/projects/nfd)

Besides officially supported platforms, NFD is known to work on: Fedora 20, CentOS 6,
Raspberry Pi, OpenWRT, FreeBSD 10.0, and several other platforms.  We are soliciting help
with documenting common problems / pitfalls in installing/using NFD on different platforms
on [NFD WiKi](http://redmine.named-data.net/projects/nfd/wiki/Wiki#Installation-experiences-for-selected-platforms).
