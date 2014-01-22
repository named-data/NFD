Requirements
---------------------

Include the following header into all .hpp and .cpp files:

/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

Recommendations
---------------

The following code style recommendations are highly advised: https://github.com/cawka/docs-ndn/blob/master/cpp.rst

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
switch, or online on Boost.Test library website (http://www.boost.org/doc/libs/1_55_0/libs/test/doc/html/).
