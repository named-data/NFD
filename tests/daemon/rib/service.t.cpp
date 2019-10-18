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

#include "rib/service.hpp"
#include "common/global.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/rib-io-fixture.hpp"

#include <boost/property_tree/info_parser.hpp>
#include <sstream>

namespace nfd {
namespace rib {
namespace tests {

using namespace nfd::tests;

class RibServiceFixture : public RibIoFixture
{
protected:
  static ConfigSection
  makeSection(const std::string& text, bool wantUnixSocketPath = true)
  {
    std::istringstream is(text);
    ConfigSection section;
    boost::property_tree::read_info(is, section);
    if (wantUnixSocketPath)
      section.put("face_system.unix.path", "/dev/null");
    return section;
  }

protected:
  ndn::KeyChain m_ribKeyChain;
};

BOOST_FIXTURE_TEST_SUITE(TestService, RibServiceFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  auto section = makeSection("");

  BOOST_CHECK_THROW(Service::get(), std::logic_error);
  BOOST_CHECK_THROW(Service(section, m_ribKeyChain), std::logic_error);

  runOnRibIoService([&] {
    {
      BOOST_CHECK_THROW(Service::get(), std::logic_error);
      Service ribService(section, m_ribKeyChain);
      BOOST_CHECK_EQUAL(&ribService, &Service::get());
    }
    BOOST_CHECK_THROW(Service::get(), std::logic_error);
    Service ribService(section, m_ribKeyChain);
    BOOST_CHECK_THROW(Service(section, m_ribKeyChain), std::logic_error);
  });
  poll();
}

BOOST_AUTO_TEST_SUITE(ProcessConfig)

BOOST_AUTO_TEST_CASE(EmptyLocalhostSecurity)
{
  const std::string CONFIG = R"CONFIG(
    rib
    {
      localhost_security
    }
  )CONFIG";

  runOnRibIoService([&] {
    BOOST_CHECK_NO_THROW(Service(makeSection(CONFIG), m_ribKeyChain));
  });
  poll();
}

BOOST_AUTO_TEST_CASE(EmptyPrefixAnnouncementValidation)
{
  const std::string CONFIG = R"CONFIG(
    rib
    {
      prefix_announcement_validation
    }
  )CONFIG";

  runOnRibIoService([&] {
    BOOST_CHECK_NO_THROW(Service(makeSection(CONFIG), m_ribKeyChain));
  });
  poll();
}

BOOST_AUTO_TEST_CASE(LocalhopAndPropagate)
{
  const std::string CONFIG = R"CONFIG(
    rib
    {
      localhost_security
      {
        trust-anchor
        {
          type any
        }
      }
      localhop_security
      {
        trust-anchor
        {
          type any
        }
      }
      auto_prefix_propagate
    }
  )CONFIG";

  runOnRibIoService([&] {
    BOOST_CHECK_EXCEPTION(Service(makeSection(CONFIG), m_ribKeyChain), ConfigFile::Error,
                          [] (const auto& e) {
                            return e.what() == "localhop_security and auto_prefix_propagate "
                                               "cannot be enabled at the same time"s;
                          });
  });
  poll();
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_SUITE_END() // TestService

} // namespace tests
} // namespace rib
} // namespace nfd
