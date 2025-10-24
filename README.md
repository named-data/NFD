<div align="center">

[<img alt height="65" src="docs/ndn-logo.svg"/>](https://named-data.net/)

# NFD: Named Data Networking Forwarding Daemon

</div>

![Latest version](https://img.shields.io/github/v/tag/named-data/NFD?label=Latest%20version)
![Language](https://img.shields.io/badge/C%2B%2B-17-blue)
[![CI](https://github.com/named-data/NFD/actions/workflows/ci.yml/badge.svg)](https://github.com/named-data/NFD/actions/workflows/ci.yml)
[![Docker](https://github.com/named-data/NFD/actions/workflows/docker.yml/badge.svg)](https://github.com/named-data/NFD/actions/workflows/docker.yml)
[![Docs](https://github.com/named-data/NFD/actions/workflows/docs.yml/badge.svg)](https://github.com/named-data/NFD/actions/workflows/docs.yml)

## Overview

NFD is a network forwarder that implements and evolves together with the Named
Data Networking (NDN) [protocol](https://docs.named-data.net/NDN-packet-spec/).
Since the initial public release in 2014, NFD has been a core component of the
[NDN Platform](https://named-data.net/codebase/platform/).

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

## Documentation

See [`docs/INSTALL.rst`](docs/INSTALL.rst) for compilation and installation instructions.

Extensive documentation is available on NFD's [homepage](https://docs.named-data.net/NFD/).

## Reporting bugs

Please submit any bug reports or feature requests to the
[NFD issue tracker](https://redmine.named-data.net/projects/nfd/issues).

## Contributing

NFD is developed by a community effort.  Although the first release was mostly done by the
members of [NSF-sponsored NDN project team](https://named-data.net/project/participants/),
it already contains significant contributions from people outside the project team (see
[`AUTHORS.md`](AUTHORS.md)).  We strongly encourage participation from all interested parties,
since broader community support is key for NDN to succeed as a new Internet architecture.

Contributions to NFD are greatly appreciated and can be made through our
[Gerrit code review site](https://gerrit.named-data.net/).
If you are new to the NDN software community, please read our
[Contributor's Guide](https://github.com/named-data/.github/blob/main/CONTRIBUTING.md)
and [`README-dev.md`](README-dev.md) to get started.

## License

NFD is free software distributed under the GNU General Public License version 3.
See [`COPYING.md`](COPYING.md) for details.

NFD contains third-party software, licensed under the following licenses:

* *CityHash* is licensed under the
  [MIT license](https://github.com/google/cityhash/blob/master/COPYING)
* The *WebSocket++* library is licensed under the
  [3-clause BSD license](https://github.com/zaphoyd/websocketpp/blob/0.8.1/COPYING)
* The *waf* build system is licensed under the [3-clause BSD license](waf)
