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

#include "mgmt/rib-manager.hpp"

#include "tests/test-common.hpp"
#include "tests/key-chain-fixture.hpp"
#include "tests/daemon/global-io-fixture.hpp"
#include "tests/daemon/rib/fib-updates-common.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

#include <boost/property_tree/info_parser.hpp>

namespace nfd {
namespace tests {

using rib::Route;

static ConfigSection
makeSection(const std::string& config)
{
  std::istringstream inputStream(config);
  ConfigSection section;
  boost::property_tree::read_info(inputStream, section);
  return section;
}

class RibManagerSlAnnounceFixture : public GlobalIoTimeFixture, public KeyChainFixture
{
public:
  using SlAnnounceResult = RibManager::SlAnnounceResult;

  RibManagerSlAnnounceFixture()
    : m_face(g_io, m_keyChain)
    , m_nfdController(m_face, m_keyChain)
    , m_dispatcher(m_face, m_keyChain)
    , m_fibUpdater(rib, m_nfdController)
    , m_trustedSigner(m_keyChain.createIdentity("/trusted", ndn::RsaKeyParams()))
    , m_untrustedSigner(m_keyChain.createIdentity("/untrusted", ndn::RsaKeyParams()))
  {
    // Face, Controller, Dispatcher are irrelevant to SlAnnounce functions but required by
    // RibManager construction, so they are private. RibManager is a pointer to avoid code style
    // rule 1.4 violation.
    manager = make_unique<RibManager>(rib, m_face, m_keyChain, m_nfdController, m_dispatcher);

    loadDefaultPaConfig();
  }

  template<typename ...T>
  ndn::PrefixAnnouncement
  makeTrustedAnn(T&&... args)
  {
    return signPrefixAnn(makePrefixAnn(std::forward<T>(args)...), m_keyChain, m_trustedSigner);
  }

  template<typename ...T>
  ndn::PrefixAnnouncement
  makeUntrustedAnn(T&&... args)
  {
    return signPrefixAnn(makePrefixAnn(std::forward<T>(args)...), m_keyChain, m_untrustedSigner);
  }

  /** \brief Invoke manager->slAnnounce and wait for result.
   */
  SlAnnounceResult
  slAnnounceSync(const ndn::PrefixAnnouncement& pa, uint64_t faceId, time::milliseconds maxLifetime)
  {
    optional<SlAnnounceResult> result;
    manager->slAnnounce(pa, faceId, maxLifetime,
      [&] (RibManager::SlAnnounceResult res) {
        BOOST_CHECK(!result);
        result = res;
      });

    g_io.poll();
    BOOST_CHECK(result);
    return result.value_or(SlAnnounceResult::ERROR);
  }

  /** \brief Invoke manager->slRenew and wait for result.
   */
  SlAnnounceResult
  slRenewSync(const Name& name, uint64_t faceId, time::milliseconds maxLifetime)
  {
    optional<SlAnnounceResult> result;
    manager->slRenew(name, faceId, maxLifetime,
      [&] (RibManager::SlAnnounceResult res) {
        BOOST_CHECK(!result);
        result = res;
      });

    g_io.poll();
    BOOST_CHECK(result);
    return result.value_or(SlAnnounceResult::ERROR);
  }

  /** \brief Invoke manager->slFindAnn and wait for result.
   */
  optional<ndn::PrefixAnnouncement>
  slFindAnnSync(const Name& name)
  {
    optional<optional<ndn::PrefixAnnouncement>> result;
    manager->slFindAnn(name,
      [&] (optional<ndn::PrefixAnnouncement> found) {
        BOOST_CHECK(!result);
        result = found;
      });

    g_io.poll();
    BOOST_CHECK(result);
    return result.value_or(nullopt);
  }

  /** \brief Lookup a route with PREFIXANN origin.
   */
  Route*
  findAnnRoute(const Name& name, uint64_t faceId)
  {
    Route routeQuery;
    routeQuery.faceId = faceId;
    routeQuery.origin = ndn::nfd::ROUTE_ORIGIN_PREFIXANN;
    return rib.find(name, routeQuery);
  }

private:
  void
  loadDefaultPaConfig()
  {
    const std::string CONFIG = R"CONFIG(
      trust-anchor
      {
        type any
      }
    )CONFIG";
    manager->applyPaConfig(makeSection(CONFIG), "default");
  }

public:
  rib::Rib rib;
  unique_ptr<RibManager> manager;

private:
  ndn::util::DummyClientFace m_face;
  ndn::nfd::Controller m_nfdController;
  Dispatcher m_dispatcher;
  rib::tests::MockFibUpdater m_fibUpdater;

  ndn::security::SigningInfo m_trustedSigner;
  ndn::security::SigningInfo m_untrustedSigner;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_AUTO_TEST_SUITE(TestRibManager)
BOOST_FIXTURE_TEST_SUITE(SlAnnounce, RibManagerSlAnnounceFixture)

BOOST_AUTO_TEST_CASE(AnnounceWithDefaultConfig)
{
  auto pa = makeTrustedAnn("/fMXN7UeB", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 3275, 1_h), SlAnnounceResult::OK);
  BOOST_CHECK(findAnnRoute("/fMXN7UeB", 3275) != nullptr);

  auto pa2 = makeUntrustedAnn("/1nzAe0Y4", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa2, 2959, 1_h), SlAnnounceResult::OK);
  BOOST_CHECK(findAnnRoute("/1nzAe0Y4", 2959) != nullptr);
}

BOOST_AUTO_TEST_CASE(AnnounceWithEmptyConfig)
{
  manager->applyPaConfig(makeSection(""), "empty");

  auto pa = makeTrustedAnn("/fMXN7UeB", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 3275, 1_h), SlAnnounceResult::VALIDATION_FAILURE);
  BOOST_CHECK(findAnnRoute("/fMXN7UeB", 3275) == nullptr);

  auto pa2 = makeUntrustedAnn("/1nzAe0Y4", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa2, 2959, 1_h), SlAnnounceResult::VALIDATION_FAILURE);
  BOOST_CHECK(findAnnRoute("/1nzAe0Y4", 2959) == nullptr);
}

BOOST_AUTO_TEST_CASE(AnnounceValidationError)
{
  ConfigSection section;
  section.put("rule.id", "PA");
  section.put("rule.for", "data");
  section.put("rule.checker.type", "customized");
  section.put("rule.checker.sig-type", "rsa-sha256");
  section.put("rule.checker.key-locator.type", "name");
  section.put("rule.checker.key-locator.name", "/trusted");
  section.put("rule.checker.key-locator.relation", "is-prefix-of");
  section.put("trust-anchor.type", "base64");
  section.put("trust-anchor.base64-string", getIdentityCertificateBase64("/trusted"));
  manager->applyPaConfig(section, "trust-schema.section");

  auto pa = makeUntrustedAnn("/1nzAe0Y4", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 2959, 1_h), SlAnnounceResult::VALIDATION_FAILURE);

  BOOST_CHECK(findAnnRoute("/1nzAe0Y4", 2959) == nullptr);
}

BOOST_AUTO_TEST_CASE(AnnounceInsert_AnnLifetime)
{
  auto pa = makeTrustedAnn("/EHJYmJz9", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 1641, 2_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/EHJYmJz9", 1641);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 1_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 1_h);
}

BOOST_AUTO_TEST_CASE(AnnounceInsert_ArgLifetime)
{
  auto pa = makeTrustedAnn("/BU9Fec9E", 2_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 1282, 1_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/BU9Fec9E", 1282);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 2_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 1_h);
}

BOOST_AUTO_TEST_CASE(AnnounceReplace)
{
  auto pa = makeTrustedAnn("/HsBFGvL3", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 2813, 1_h), SlAnnounceResult::OK);

  pa = makeTrustedAnn("/HsBFGvL3", 2_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 2813, 2_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/HsBFGvL3", 2813);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 2_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 2_h);
}

BOOST_AUTO_TEST_CASE(AnnounceExpired)
{
  auto pa = makeTrustedAnn("/awrVv6V7", 1_h, std::make_pair(-3_h, -1_h));
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 9087, 1_h), SlAnnounceResult::EXPIRED);

  BOOST_CHECK(findAnnRoute("/awrVv6V7", 9087) == nullptr);
}

BOOST_AUTO_TEST_CASE(RenewNotFound)
{
  BOOST_CHECK_EQUAL(slRenewSync("IAYigN73", 1070, 1_h), SlAnnounceResult::NOT_FOUND);

  BOOST_CHECK(findAnnRoute("/IAYigN73", 1070) == nullptr);
}

BOOST_AUTO_TEST_CASE(RenewProlong_ArgLifetime)
{
  auto pa = makeTrustedAnn("/P2IYFqtr", 4_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 4506, 2_h), SlAnnounceResult::OK);
  advanceClocks(1_h); // Route has 1_h remaining lifetime

  BOOST_CHECK_EQUAL(slRenewSync("/P2IYFqtr/2321", 4506, 2_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/P2IYFqtr", 4506);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 3_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 2_h); // set by slRenew
}

BOOST_AUTO_TEST_CASE(RenewProlong_AnnLifetime)
{
  auto pa = makeTrustedAnn("/be01Yiba", 4_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 1589, 2_h), SlAnnounceResult::OK);
  advanceClocks(1_h); // Route has 1_h remaining lifetime

  BOOST_CHECK_EQUAL(slRenewSync("/be01Yiba/4324", 1589, 5_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/be01Yiba", 1589);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 3_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 3_h); // capped by annExpires
}

BOOST_AUTO_TEST_CASE(RenewShorten)
{
  auto pa = makeTrustedAnn("/5XCHYCAd", 4_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 3851, 4_h), SlAnnounceResult::OK);
  advanceClocks(1_h); // Route has 3_h remaining lifetime

  BOOST_CHECK_EQUAL(slRenewSync("/5XCHYCAd/98934", 3851, 1_h), SlAnnounceResult::OK);

  Route* route = findAnnRoute("/5XCHYCAd", 3851);
  BOOST_REQUIRE(route != nullptr);
  BOOST_CHECK_EQUAL(route->annExpires, time::steady_clock::now() + 3_h);
  BOOST_CHECK_EQUAL(route->expires.value(), time::steady_clock::now() + 1_h); // set by slRenew
}

BOOST_AUTO_TEST_CASE(RenewShorten_Zero)
{
  auto pa = makeTrustedAnn("/cdQ7KPNw", 4_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 8031, 4_h), SlAnnounceResult::OK);
  advanceClocks(1_h); // Route has 3_h remaining lifetime

  BOOST_CHECK_EQUAL(slRenewSync("/cdQ7KPNw/8023", 8031, 0_s), SlAnnounceResult::EXPIRED);

  BOOST_CHECK(findAnnRoute("/cdQ7KPNw", 8031) == nullptr);
}

BOOST_AUTO_TEST_CASE(FindExisting)
{
  auto pa = makeTrustedAnn("/JHugsjjr", 1_h);
  BOOST_CHECK_EQUAL(slAnnounceSync(pa, 2363, 1_h), SlAnnounceResult::OK);

  auto found = slFindAnnSync("/JHugsjjr");
  BOOST_REQUIRE(found);
  BOOST_CHECK_EQUAL(found->getAnnouncedName(), "/JHugsjjr");
  BOOST_CHECK(found->getData());

  auto found2 = slFindAnnSync("/JHugsjjr/StvXhKR5");
  BOOST_CHECK(found == found2);
}

BOOST_AUTO_TEST_CASE(FindNew)
{
  Route route;
  route.faceId = 1367;
  route.origin = ndn::nfd::ROUTE_ORIGIN_APP;
  rib.insert("/dLY1pRhR", route);

  auto pa = slFindAnnSync("/dLY1pRhR/3qNK9Ngn");
  BOOST_REQUIRE(pa);
  BOOST_CHECK_EQUAL(pa->getAnnouncedName(), "/dLY1pRhR");
}

BOOST_AUTO_TEST_CASE(FindNone)
{
  auto pa = slFindAnnSync("/2YNeYuV2");
  BOOST_CHECK(!pa);
}

BOOST_AUTO_TEST_SUITE_END() // SlAnnounce
BOOST_AUTO_TEST_SUITE_END() // TestRibManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
