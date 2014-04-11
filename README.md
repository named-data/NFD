NFD - Named Data Networking Forwarding Daemon
==============================================================

NFD is the network forwarder that implements the Named Data Networking (NDN)
[protocol](http://named-data.net/doc/ndn-tlv/) and evolves together with the NDN protocol.

The main design goal of NFD development is to support diverse experimention of NDN
research.  The design emphasizes on modularity and extensibility to allow easy experiments
with new protocol features, new algorithms, and new applications.  We have not optimized
the code for performance, but it can be achieved within the same framework by trying out
different data structures and algorithms.

NFD is open, free, and developed by a community effort.  Although most code of this first
release was done by the NSF-sponsored NDN project team, it also contains significant
contributions by people outside the project team.  We strongly encourage contributions from
all interested parties, since broader community support is key to NDN’s success as a new
Internet architecture.  Bug reports and feedbacks can be made through
[Redmine site](http://redmine.named-data.net/projects/nfd) and the
[ndn-interest mailing list](http://www.lists.cs.ucla.edu/mailman/listinfo/ndn-interest).

The design and development of NFD benefited from our experiences with CCNx. However NFD
does not maintain compatibility with [CCNx](http://www.ccnx.org).

Documentation
-------------

For more information refer to

* [Installation instruction `docs/INSTALL.rst`](https://github.com/named-data/NFD/blob/master/docs/INSTALL.rst),
* [Hints on configuration `docs/README.rst`](https://github.com/named-data/NFD/blob/master/docs/README.rst)
* Other documentation in [`docs/`](https://github.com/named-data/NFD/blob/master/docs) folder.
