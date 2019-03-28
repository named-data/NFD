/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "rib/readvertise/host-to-gateway-readvertise-policy.hpp"

#include "tests/test-common.hpp"
#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

class HostToGatewayReadvertisePolicyFixture : public GlobalIoFixture, public KeyChainFixture
{
public:
  static RibRouteRef
  makeNewRoute(const Name& prefix)
  {
    auto entry = make_shared<RibEntry>();
    entry->setName(prefix);

    Route route;
    auto routeIt = entry->insertRoute(route).first;
    return RibRouteRef{entry, routeIt};
  }

  shared_ptr<HostToGatewayReadvertisePolicy>
  makePolicy(const ConfigSection& section = ConfigSection())
  {
    return make_shared<HostToGatewayReadvertisePolicy>(m_keyChain, section);
  }
};

BOOST_AUTO_TEST_SUITE(Readvertise)
BOOST_FIXTURE_TEST_SUITE(TestHostToGatewayReadvertisePolicy, HostToGatewayReadvertisePolicyFixture)

BOOST_AUTO_TEST_CASE(PrefixToAdvertise)
{
  BOOST_REQUIRE(addIdentity("/A"));
  BOOST_REQUIRE(addIdentity("/A/B"));
  BOOST_REQUIRE(addIdentity("/C/nrd"));

  auto test = [this] (Name routeName, optional<ReadvertiseAction> expectedAction) {
    auto policy = makePolicy();
    auto action = policy->handleNewRoute(makeNewRoute(routeName));

    if (expectedAction) {
      BOOST_REQUIRE(action);
      BOOST_CHECK_EQUAL(action->prefix, expectedAction->prefix);
      BOOST_REQUIRE_EQUAL(action->signer, expectedAction->signer);
    }
    else {
      BOOST_REQUIRE(!action);
    }
  };

  test("/D/app", nullopt);
  test("/A/B/app", ReadvertiseAction{"/A", ndn::security::signingByIdentity("/A")});
  test("/C/nrd", ReadvertiseAction{"/C", ndn::security::signingByIdentity("/C/nrd")});
}

BOOST_AUTO_TEST_CASE(DontReadvertise)
{
  auto policy = makePolicy();
  BOOST_REQUIRE(!policy->handleNewRoute(makeNewRoute("/localhost/test")));
  BOOST_REQUIRE(!policy->handleNewRoute(makeNewRoute("/localhop/nfd")));
}

BOOST_AUTO_TEST_CASE(LoadRefreshInterval)
{
  auto policy = makePolicy();
  BOOST_CHECK_EQUAL(policy->getRefreshInterval(), 25_s); // default setting is 25

  ConfigSection section;
  section.put("refresh_interval_wrong", 10);
  policy = makePolicy(section);
  BOOST_CHECK_EQUAL(policy->getRefreshInterval(), 25_s); // wrong formate

  section.put("refresh_interval", 10);
  policy = makePolicy(section);
  BOOST_CHECK_EQUAL(policy->getRefreshInterval(), 10_s);
}

BOOST_AUTO_TEST_SUITE_END() // TestHostToGatewayReadvertisePolicy
BOOST_AUTO_TEST_SUITE_END() // Readvertise

} // namespace tests
} // namespace rib
} // namespace nfd
