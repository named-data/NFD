/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "face/face-system.hpp"
#include "face-system-fixture.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestFaceSystem, FaceSystemFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

class DummyProtocolFactory : public ProtocolFactory
{
public:
  DummyProtocolFactory(const CtorParams& params)
    : ProtocolFactory(params)
  {
  }

  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) final
  {
    processConfigHistory.push_back({configSection, context.isDryRun,
                                    context.generalConfig.wantCongestionMarking});
    if (!context.isDryRun) {
      this->providedSchemes = this->newProvidedSchemes;
    }
  }

  void
  createFace(const CreateFaceRequest& req,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) final
  {
    BOOST_FAIL("createFace should not be called");
  }

  std::vector<shared_ptr<const Channel>>
  getChannels() const final
  {
    BOOST_FAIL("getChannels should not be called");
    return {};
  }

public:
  struct ProcessConfigArgs
  {
    OptionalConfigSection configSection;
    bool isDryRun;
    bool wantCongestionMarking;
  };
  std::vector<ProcessConfigArgs> processConfigHistory;

  std::set<std::string> newProvidedSchemes;
};

BOOST_AUTO_TEST_CASE(Normal)
{
  faceSystem.m_factories["f1"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  faceSystem.m_factories["f2"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  auto f1 = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("f1"));
  auto f2 = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("f2"));

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      general
      {
        enable_congestion_marking yes
      }
      f1
      {
        key v1
      }
      f2
      {
        key v2
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK(f1->processConfigHistory.back().isDryRun);
  BOOST_CHECK(f1->processConfigHistory.back().wantCongestionMarking);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK(f2->processConfigHistory.back().isDryRun);
  BOOST_CHECK(f2->processConfigHistory.back().wantCongestionMarking);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");

  parseConfig(CONFIG, false);
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK(!f1->processConfigHistory.back().isDryRun);
  BOOST_CHECK(f1->processConfigHistory.back().wantCongestionMarking);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().configSection->get<std::string>("key"), "v1");
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK(!f2->processConfigHistory.back().isDryRun);
  BOOST_CHECK(f2->processConfigHistory.back().wantCongestionMarking);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().configSection->get<std::string>("key"), "v2");
}

BOOST_AUTO_TEST_CASE(OmittedSection)
{
  faceSystem.m_factories["f1"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  faceSystem.m_factories["f2"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  auto f1 = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("f1"));
  auto f2 = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("f2"));

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, true);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 1);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, true);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);

  parseConfig(CONFIG, false);
  BOOST_REQUIRE_EQUAL(f1->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f1->processConfigHistory.back().isDryRun, false);
  BOOST_REQUIRE_EQUAL(f2->processConfigHistory.size(), 2);
  BOOST_CHECK_EQUAL(f2->processConfigHistory.back().isDryRun, false);
  BOOST_CHECK(!f2->processConfigHistory.back().configSection);
}

BOOST_AUTO_TEST_CASE(UnknownSection)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f0
      {
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ChangeProvidedSchemes)
{
  faceSystem.m_factories["f1"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  auto f1 = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("f1"));

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      f1
      {
      }
    }
  )CONFIG";

  f1->newProvidedSchemes.insert("s1");
  f1->newProvidedSchemes.insert("s2");
  parseConfig(CONFIG, false);
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s2"), f1);

  f1->newProvidedSchemes.erase("s2");
  f1->newProvidedSchemes.insert("s3");
  parseConfig(CONFIG, false);
  BOOST_CHECK(faceSystem.getFactoryByScheme("f1") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s1"), f1);
  BOOST_CHECK(faceSystem.getFactoryByScheme("s2") == nullptr);
  BOOST_CHECK_EQUAL(faceSystem.getFactoryByScheme("s3"), f1);
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_SUITE_END() // TestFaceSystem
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace face
} // namespace nfd
