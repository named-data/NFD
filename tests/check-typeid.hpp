/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
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

/** \file
 *  \brief provides unit testing tools to validate runtime type information
 */

#ifndef NFD_TESTS_CHECK_TYPEID_HPP
#define NFD_TESTS_CHECK_TYPEID_HPP

#include "boost-test.hpp"
#include <typeinfo>

/** \def NFD_TYPEID_NAME(x)
 *  \return type name of typeid(x) as std::string
 */
#if BOOST_VERSION >= 105600
#include <boost/core/demangle.hpp>
#define NFD_TYPEID_NAME(x) (::boost::core::demangle(typeid(x).name()))
#else
#define NFD_TYPEID_NAME(x) (::std::string(typeid(x).name()))
#endif

/** (implementation detail)
 */
#define NFD_TEST_TYPEID_REL(level, a, op, b) \
  BOOST_##level##_MESSAGE((typeid(a) op typeid(b)), \
                          "expect typeid(" #a ") " #op " typeid(" #b ") [" << \
                          NFD_TYPEID_NAME(a) << " " #op " " << NFD_TYPEID_NAME(b) << "]")

/** (implementation detail)
 */
#define NFD_TEST_TYPEID_EQUAL(level, a, b) NFD_TEST_TYPEID_REL(level, a, ==, b)

/** (implementation detail)
 */
#define NFD_TEST_TYPEID_NE(level, a, b) NFD_TEST_TYPEID_REL(level, a, !=, b)

/** \brief equivalent to BOOST_REQUIRE(typeid(a) == typeid(b))
 */
#define NFD_REQUIRE_TYPEID_EQUAL(a, b) NFD_TEST_TYPEID_EQUAL(REQUIRE, a, b)

/** \brief equivalent to BOOST_REQUIRE(typeid(a) != typeid(b))
 */
#define NFD_REQUIRE_TYPEID_NE(a, b) NFD_TEST_TYPEID_NE(REQUIRE, a, b)

/** \brief equivalent to BOOST_CHECK(typeid(a) == typeid(b))
 */
#define NFD_CHECK_TYPEID_EQUAL(a, b) NFD_TEST_TYPEID_EQUAL(CHECK, a, b)

/** \brief equivalent to BOOST_CHECK(typeid(a) != typeid(b))
 */
#define NFD_CHECK_TYPEID_NE(a, b) NFD_TEST_TYPEID_NE(CHECK, a, b)

/** \brief equivalent to BOOST_WARN(typeid(a) == typeid(b))
 */
#define NFD_WARN_TYPEID_EQUAL(a, b) NFD_TEST_TYPEID_EQUAL(WARN, a, b)

/** \brief equivalent to BOOST_WARN(typeid(a) != typeid(b))
 */
#define NFD_WARN_TYPEID_NE(a, b) NFD_TEST_TYPEID_NE(WARN, a, b)

#endif // NFD_TESTS_CHECK_TYPEID_HPP
