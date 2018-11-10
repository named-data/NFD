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

#include "face/netdev-bound.hpp"
#include "face-system-fixture.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestNetdevBound, FaceSystemFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

BOOST_AUTO_TEST_CASE(Normal)
{
  faceSystem.m_factories["pf"] = make_unique<DummyProtocolFactory>(faceSystem.makePFCtorParams());
  auto pf = static_cast<DummyProtocolFactory*>(faceSystem.getFactoryById("pf"));
  pf->newProvidedSchemes.insert("udp4+dev");

  const std::string CONFIG = R"CONFIG(
    face_system
    {
      pf
      {
      }
      netdev_bound
      {
        rule
        {
          remote udp4://192.0.2.1:6363
          remote udp4://192.0.2.2:6363
          whitelist
          {
            *
          }
          blacklist
          {
            ifname wlan0
          }
        }
        rule
        {
          remote udp4://192.0.2.3:6363
          remote udp4://192.0.2.4:6363
          whitelist
          {
            ifname eth0
          }
          blacklist
          {
          }
        }
        rule
        {
          remote udp4://192.0.2.5:6363
        }
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);
}

BOOST_AUTO_TEST_CASE(NonCanonicalRemote)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      netdev_bound
      {
        rule
        {
          remote udp://192.0.2.1
        }
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_SUITE_END() // TestNetdevBound
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
