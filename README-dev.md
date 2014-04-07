Requirements
---------------------

Include the following header into all `.hpp` and `.cpp` files:

    /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
    /**
     * Copyright (c) 2014  Regents of the University of California,
     *                     Arizona Board of Regents,
     *                     Colorado State University,
     *                     University Pierre & Marie Curie, Sorbonne University,
     *                     Washington University in St. Louis,
     *                     Beijing Institute of Technology
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
     ////// [optional part] //////
     *
     * \author Author's Name <email@domain>
     * \author Other Author's Name <another.email@domain>
     ////// [end of optional part] //////
     **/

Recommendations
---------------

NFD code is subject to the code style, defined here:
http://redmine.named-data.net/projects/nfd/wiki/CodeStyle

Running unit-tests
------------------

To run unit tests, NFD needs to be configured and build with unit test support:

    ./waf configure --with-tests
    ./waf

The simplest way to run tests, is just to run the compiled binary without any parameters:

    ./build/unit-tests

However, Boost.Test framework is very flexible and allow a number of
run-time customization of what tests should be run.  For example, it
is possible to choose to run only specific test suite or only a
specific test case within a suite:

    # Run only skeleton test suite (see tests/test-skeleton.cpp)
    ./build/unit-tests -t TestSkeleton

    # Run only test cast Test1 from skeleton suite
    ./build/unit-tests -t TestSkeleton/Test1

By default, Boost.Test framework will produce verbose output only when
test case fails.  If it is desired to see verbose output (result of
each test assertion), add ``-l all`` option to ``./build/unit-tests``
command:

    ./build/unit-tests -l all

There are many more command line options available, information about
which can be obtained either from the command line using ``--help``
switch, or online on Boost.Test library website
(http://www.boost.org/doc/libs/1_48_0/libs/test/doc/html/).
