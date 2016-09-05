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

#include "face/unix-stream-factory.hpp"

#include "factory-test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

#define CHANNEL_PATH1 "unix-stream-test.1.sock"
#define CHANNEL_PATH2 "unix-stream-test.2.sock"

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUnixStreamFactory, BaseFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(ChannelMap)
{
  UnixStreamFactory factory;

  shared_ptr<UnixStreamChannel> channel1 = factory.createChannel(CHANNEL_PATH1);
  shared_ptr<UnixStreamChannel> channel1a = factory.createChannel(CHANNEL_PATH1);
  BOOST_CHECK_EQUAL(channel1, channel1a);
  std::string channel1uri = channel1->getUri().toString();
  BOOST_CHECK_EQUAL(channel1uri.find("unix:///"), 0); // third '/' is the path separator
  BOOST_CHECK_EQUAL(channel1uri.rfind(CHANNEL_PATH1),
                    channel1uri.size() - std::string(CHANNEL_PATH1).size());

  shared_ptr<UnixStreamChannel> channel2 = factory.createChannel(CHANNEL_PATH2);
  BOOST_CHECK_NE(channel1, channel2);
}

BOOST_AUTO_TEST_CASE(GetChannels)
{
  UnixStreamFactory factory;
  BOOST_CHECK(factory.getChannels().empty());

  std::vector<shared_ptr<const Channel>> expectedChannels;
  expectedChannels.push_back(factory.createChannel(CHANNEL_PATH1));
  expectedChannels.push_back(factory.createChannel(CHANNEL_PATH2));

  for (const auto& channel : factory.getChannels()) {
    auto pos = std::find(expectedChannels.begin(), expectedChannels.end(), channel);
    BOOST_REQUIRE(pos != expectedChannels.end());
    expectedChannels.erase(pos);
  }

  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  UnixStreamFactory factory;

  createFace(factory,
             FaceUri("unix:///var/run/nfd.sock"),
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("unix:///var/run/nfd.sock"),
             ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("unix:///var/run/nfd.sock"),
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});
}

BOOST_AUTO_TEST_SUITE_END() // TestUnixStreamFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
