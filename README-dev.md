Notes for NFD developers
========================

If you are new to the NDN software community, please read the
[Contributor's Guide](https://github.com/named-data/NFD/blob/master/CONTRIBUTING.md)

Requirements
------------

Contributions to NFD must be licensed under GPL 3.0 or compatible license.  If you are
choosing GPL 3.0, please use the following license boilerplate in all `.hpp` and `.cpp`
files:

Include the following license boilerplate into all `.hpp` and `.cpp` files:

    /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
    /*
     * Copyright (c) [Year(s)],  [Copyright Holder(s)].
     *
     * This file is part of NFD (Named Data Networking Forwarding Daemon).
     * See AUTHORS.md for complete list of NFD authors and contributors.
     *
     * NFD is free software: you can redistribute it and/or modify it under the terms
     * of the GNU General Public License as published by the Free Software Foundation,
     * either version 3 of the License, or (at your option) any later version.
     *
     * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
     * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     * PURPOSE.  See the GNU General Public License for more details.
     *
     * You should have received a copy of the GNU General Public License along with
     * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
     */

If you are affiliated to an NSF-supported NDN project institution, please use the [NDN Team License
Boilerplate](https://redmine.named-data.net/projects/nfd/wiki/NDN_Team_License_Boilerplate_(NFD)).

Recommendations
---------------

NFD code is subject to NFD [code style](https://redmine.named-data.net/projects/nfd/wiki/CodeStyle).


Running unit-tests
------------------

To run unit tests, NFD needs to be configured and build with unit test support:

    ./waf configure --with-tests
    ./waf

The simplest way to run tests, is just to run the compiled binary without any parameters:

    # Run core tests
    ./build/unit-tests-core

    # Run  NFD daemon tests
    ./build/unit-tests-daemon

    # Run NFD RIB management tests
    ./build/unit-tests-rib

However, [Boost.Test framework](http://www.boost.org/doc/libs/1_54_0/libs/test/doc/html/index.html)
is very flexible and allows a number of run-time customization of what tests should be run.
For example, it is possible to choose to run only a specific test suite, only a specific
test case within a suite, or specific test cases within specific test suites:

    # Run only TCP Face test suite of NFD daemon tests (see tests/daemon/face/tcp.cpp)
    ./build/unit-tests-daemon -t FaceTcp

    # Run only test case EndToEnd4 from the same test suite
    ./build/unit-tests-daemon -t FaceTcp/EndToEnd4

    # Run Basic test case from all core test suites
    ./build/unit-tests-core -t */Basic

By default, Boost.Test framework will produce verbose output only when a test case fails.
If it is desired to see verbose output (result of each test assertion), add `-l all`
option to `./build/unit-tests` command.  To see test progress, you can use `-l test_suite`
or `-p` to show progress bar:

    # Show report all log messages including the passed test notification
    ./build/unit-tests-daemon -l all

    # Show test suite messages
    ./build/unit-tests-daemon -l test_suite

    # Show nothing
    ./build/unit-tests-daemon -l nothing

    # Show progress bar
    ./build/unit-tests-core -p

There are many more command line options available, information about
which can be obtained either from the command line using `--help`
switch, or online on [Boost.Test library](http://www.boost.org/doc/libs/1_54_0/libs/test/doc/html/index.html)
website.
