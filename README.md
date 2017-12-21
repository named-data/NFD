NFD - Named Data Networking Forwarding Daemon
=============================================

[![Build Status](https://travis-ci.org/named-data/NFD.svg?branch=master)](https://travis-ci.org/named-data/NFD)

For complete documentation, including step-by-step installation instructions and
tutorials, please visit the [NFD homepage](https://named-data.net/doc/NFD/).

If you are new to the NDN software community, please read README-dev.md and the
[Contributor's Guide](https://github.com/named-data/NFD/blob/master/CONTRIBUTING.md).

## Overview

NFD is a network forwarder that implements and evolves together with the Named Data
Networking (NDN) [protocol](https://named-data.net/doc/ndn-tlv/).  After the initial
release, NFD will become a core component of the
[NDN Platform](https://named-data.net/codebase/platform/) and will follow the same release
cycle.

NFD is an open and free software package licensed under GPL 3.0 license and is the
centerpiece of our committement to making NDN's core technology open and free to all
Internet users and developers.  For more information about the licensing details and
limitation, refer to
[`COPYING.md`](https://github.com/named-data/NFD/blob/master/COPYING.md).

NFD is developed by a community effort.  Although the first release was mostly done by the
members of [NSF-sponsored NDN project team](https://named-data.net/project/participants/),
it already contains significant contributions from people outside the project team (for
more details, refer to
[`AUTHORS.md`](https://github.com/named-data/NFD/blob/master/AUTHORS.md)).  We strongly
encourage participation from all interested parties, since broader community support is
key for NDN to succeed as a new Internet architecture.  Bug reports and feedback are
highly appreciated and can be made through
[Redmine site](https://redmine.named-data.net/projects/nfd) and the
[ndn-interest mailing list](http://www.lists.cs.ucla.edu/mailman/listinfo/ndn-interest).

The main design goal of NFD is to support diverse experimentation of NDN technology.  The
design emphasizes *modularity* and *extensibility* to allow easy experiments with new
protocol features, algorithms, new applications.  We have not fully optimized the code for
performance.  The intention is that performance optimizations are one type of experiments
that developers can conduct by trying out different data structures and different
algorithms; over time, better implementations may emerge within the same design framework.

NFD will keep evolving in three aspects: improvement of the modularity framework, keeping
up with the NDN protocol spec, and addition of other new features. We hope to keep the
modular framework stable and lean, allowing researchers to implement and experiment with
various features, some of which may eventually work into the protocol spec.
