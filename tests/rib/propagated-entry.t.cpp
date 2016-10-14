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

#include "rib/propagated-entry.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace rib {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestPropagatedEntry, nfd::tests::BaseFixture)

BOOST_AUTO_TEST_CASE(Identity)
{
  PropagatedEntry entry;
  BOOST_CHECK(entry.m_signingIdentity.empty());

  entry.setSigningIdentity("/test");
  BOOST_CHECK_EQUAL(entry.m_signingIdentity, "/test");
  BOOST_CHECK_EQUAL(entry.getSigningIdentity(), "/test");
}

BOOST_AUTO_TEST_CASE(Start)
{
  PropagatedEntry entry;
  BOOST_CHECK(!entry.isPropagating());

  entry.startPropagation();
  BOOST_CHECK_EQUAL(PropagationStatus::PROPAGATING, entry.m_propagationStatus);
  BOOST_CHECK(entry.isPropagating());
}

BOOST_AUTO_TEST_CASE(Succeed)
{
  PropagatedEntry entry;
  entry.succeed(nullptr);
  BOOST_CHECK_EQUAL(PropagationStatus::PROPAGATED, entry.m_propagationStatus);
  BOOST_CHECK(entry.isPropagated());
}

BOOST_AUTO_TEST_CASE(Fail)
{
  PropagatedEntry entry;
  entry.fail(nullptr);
  BOOST_CHECK_EQUAL(PropagationStatus::PROPAGATE_FAIL, entry.m_propagationStatus);
  BOOST_CHECK(entry.isPropagateFail());
}

BOOST_AUTO_TEST_CASE(Initialize)
{
  PropagatedEntry entry;
  entry.startPropagation();
  BOOST_CHECK_EQUAL(PropagationStatus::PROPAGATING, entry.m_propagationStatus);

  entry.initialize();
  BOOST_CHECK_EQUAL(PropagationStatus::NEW, entry.m_propagationStatus);
}

BOOST_AUTO_TEST_SUITE_END() // TestPropagatedEntry

} // namespace tests
} // namespace rib
} // namespace nfd
