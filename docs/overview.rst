NFD Overview
============

.. toctree::
   :maxdepth: 2

   RELEASE_NOTES

NDN Forwarding Daemon (NFD) is a network forwarder that implements and evolves together
with the Named Data Networking (NDN) `protocol
<http://named-data.net/doc/ndn-tlv/>`__. After the initial release, NFD will become a core
component of the `NDN Platform <http://named-data.net/codebase/platform/>`__ and will
follow the same release cycle.

NFD is developed by a community effort. Although the first release was mostly done by the
members of `NSF-sponsored NDN project team
<http://named-data.net/project/participants/>`__, it already contains significant
contributions from people outside the project team (for more details, refer to `AUTHORS.md
<https://github.com/named-data/NFD/blob/master/AUTHORS.md>`__).  We strongly encourage
participation from all interested parties, since broader community support is key for NDN
to succeed as a new Internet architecture. Bug reports and feedback are highly
appreciated and can be made through `Redmine site
<http://redmine.named-data.net/projects/nfd>`__ and the `ndn-interest mailing list
<http://www.lists.cs.ucla.edu/mailman/listinfo/ndn-interest>`__.

The main design goal of NFD is to support diverse experimentation of NDN technology. The
design emphasizes *modularity* and *extensibility* to allow easy experiments with new
protocol features, algorithms, and applications. We have not fully optimized the code for
performance.  The intention is that performance optimizations are one type of experiments
that developers can conduct by trying out different data structures and different
algorithms; over time, better implementations may emerge within the same design framework.
To facilitate such experimentation with the forwarder, the NFD team has also written a
`developer's guide <http://named-data.net/wp-content/uploads/2014/07/NFD-developer-guide.pdf>`_,
which details the current implementation and provides tips for extending all aspects of
NFD.

NFD will keep evolving in three aspects: improvement of the modularity framework, keeping
up with the NDN protocol spec, and addition of other new features. We hope to keep the
modular framework stable and lean, allowing researchers to implement and experiment
with various features, some of which may eventually work into the protocol spec.

The design and development of NFD benefited from our earlier experience with `CCNx
<http://www.ccnx.org>`__ software package. However, NFD is not in any part derived from
CCNx codebase and does not maintain compatibility with CCNx.


Major Modules of NFD
--------------------

NFD has the following major modules:

- Core
    Provides various common services shared between different NFD modules. These include
    hash computation routines, DNS resolver, config file, face monitoring, and
    several other modules.

- Faces
    Implements the NDN face abstraction on top of different lower level transport
    mechanisms.

- Tables
    Implements the Content Store (CS), the Pending Interest Table (PIT), the Forwarding
    Information Base (FIB), and other data structures to support forwarding of NDN Data
    and Interest packets.

- Forwarding
    Implements basic packet processing pathways, which interact with Faces, Tables,
    and Strategies.

  + **Strategy Support**, a major part of the forwarding module
      Implements a framework to support different forwarding strategies. It includes
      StrategyChoice, Measurements, Strategies, and hooks in the forwarding pipelines. The
      StrategyChoice records the choice of the strategy for a namespace, and Measurement
      records are used by strategies to store past performance results for namespaces.

- Management
    Implements the `NFD Management Protocol
    <http://redmine.named-data.net/projects/nfd/wiki/Management>`_, which allows
    applications to configure NFD and set/query NFD's internal states.  Protocol interaction
    is done via NDN's Interest/Data exchange between applications and NFD.

- RIB Management
    Manages the routing information base (RIB).  The RIB may be updated by different parties
    in different ways, including various routing protocols, application's prefix
    registrations, and command-line manipulation by sysadmins.  The RIB management module
    processes all these requests to generate a consistent forwarding table, and then syncs
    it up with the NFD's FIB, which contains only the minimal information needed for
    forwarding decisions. Strictly speaking RIB management is part of the NFD management
    module. However, due to its importance to the overall operations and its more complex
    processing, we make it a separate module.
