NFD - Named Data Networking Forwarding Daemon
=============================================

NFD is a network forwarder that implements and evolves together with the Named Data
Networking (NDN) `protocol <http://named-data.net/doc/ndn-tlv/>`__. After the initial
release, NFD will become a core component of the `NDN Platform
<http://named-data.net/codebase/platform/>`__ and will follow the same release cycle.

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

NFD will keep evolving in three aspects: improvement of the modularity framework, keeping
up with the NDN protocol spec, and addition of other new features. We hope to keep the
modular framework stable and lean, allowing researchers to implement and experiment
with various features, some of which may eventually work into the protocol spec.

The design and development of NFD benefited from our earlier experience with `CCNx
<http://www.ccnx.org>`__ software package. However, NFD is not in any part derived from
CCNx codebase and does not maintain compatibility with CCNx.

Downloading
-----------

NFD code can be downloaded from `GitHub git repository <https://github.com/named-data/NFD>`_.

Refer to :ref:`NFD Installation Instructions` for detailed guide on how to install NFD
from source.

NFD Documentation
-----------------

.. toctree::
   :hidden:
   :maxdepth: 2

   RELEASE_NOTES
   Installation Instruction <INSTALL>
   NFD Configuration Tips <README>
   manpages



* :ref:`NFD v0.1.0 Release Notes`

* :ref:`NFD Installation Instructions`

* :ref:`NFD Configuration Tips`

* :ref:`Manpages`

* `NFD Wiki <http://redmine.named-data.net/projects/nfd/wiki>`_

  + `NFD Management protocol <http://redmine.named-data.net/projects/nfd/wiki/Management>`_
  + `NFD Configuration file format <http://redmine.named-data.net/projects/nfd/wiki/ConfigFileFormat>`_

* `API documentation (doxygen) <doxygen/annotated.html>`_


License
-------

NFD is an open and free software package licensed under GPL 3.0 license and is the
centerpiece of our committement to making NDN's core technology open and free to all
Internet users and developers. For more information about the licensing details and
limitation, refer to `COPYING.md <https://github.com/named-data/NFD/blob/master/COPYING.md>`_.
