/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "face/websocket-factory.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceWebSocket, BaseFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  WebSocketFactory factory("19596");
  BOOST_REQUIRE_EQUAL(factory.getChannels().empty(), true);

  std::vector<shared_ptr<const Channel> > expectedChannels;

  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20070"));
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20071"));
  expectedChannels.push_back(factory.createChannel("::1", "20071"));

  std::list<shared_ptr<const Channel> > channels = factory.getChannels();
  for (std::list<shared_ptr<const Channel> >::const_iterator i = channels.begin();
       i != channels.end(); ++i)
    {
      std::vector<shared_ptr<const Channel> >::iterator pos =
        std::find(expectedChannels.begin(), expectedChannels.end(), *i);

      BOOST_REQUIRE(pos != expectedChannels.end());
      expectedChannels.erase(pos);
    }

  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
