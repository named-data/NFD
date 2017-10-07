/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "face/websocket-factory.hpp"

#include "face-system-fixture.hpp"
#include "factory-test-common.hpp"

#include <ndn-cxx/net/address-converter.hpp>

namespace nfd {
namespace face {
namespace tests {

class WebSocketFactoryFixture : public FaceSystemFactoryFixture<WebSocketFactory>
{
protected:
  shared_ptr<WebSocketChannel>
  createChannel(const std::string& localIp, const std::string& localPort)
  {
    websocket::Endpoint endpoint(ndn::ip::addressFromString(localIp),
                                 boost::lexical_cast<uint16_t>(localPort));
    return factory.createChannel(endpoint);
  }
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestWebSocketFactory, WebSocketFactoryFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

BOOST_AUTO_TEST_CASE(Normal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 yes
        enable_v6 yes
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"ws://[::]:9696"});
}

BOOST_AUTO_TEST_CASE(EnableIpv4Only)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 yes
        enable_v6 no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"ws://0.0.0.0:9696"});
}

BOOST_AUTO_TEST_CASE(UnsupportedIpv6Only)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 no
        enable_v6 yes
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(ChannelsDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen yes
        port 9696
        enable_v4 no
        enable_v6 no
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(NoListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      websocket
      {
        listen no
        port 9696
        enable_v4 yes
        enable_v6 yes
      }
    }
  )CONFIG";

  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
}

BOOST_AUTO_TEST_CASE(ChangeEndpoint)
{
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      websocket
      {
        port 9001
      }
    }
  )CONFIG";

  parseConfig(CONFIG1, false);
  checkChannelListEqual(factory, {"ws://[::]:9001"});

  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      websocket
      {
        port 9002
      }
    }
  )CONFIG";

  parseConfig(CONFIG2, false);
  checkChannelListEqual(factory, {"ws://[::]:9001", "ws://[::]:9002"});
}

BOOST_AUTO_TEST_SUITE_END() // ProcessConfig

BOOST_AUTO_TEST_CASE(GetChannels)
{
  BOOST_CHECK_EQUAL(factory.getChannels().empty(), true);

  std::set<std::string> expected;
  expected.insert(createChannel("127.0.0.1", "20070")->getUri().toString());
  expected.insert(createChannel("127.0.0.1", "20071")->getUri().toString());
  expected.insert(createChannel("::1", "20071")->getUri().toString());
  checkChannelListEqual(factory, expected);
}

BOOST_AUTO_TEST_CASE(UnsupportedCreateFace)
{
  createFace(factory,
             FaceUri("ws://127.0.0.1:20070"),
             {},
             ndn::nfd::FACE_PERSISTENCY_PERMANENT,
             false,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("ws://127.0.0.1:20070"),
             {},
             ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
             false,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});

  createFace(factory,
             FaceUri("ws://127.0.0.1:20070"),
             {},
             ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
             false,
             false,
             {CreateFaceExpectedResult::FAILURE, 406, "Unsupported protocol"});
}

BOOST_AUTO_TEST_SUITE_END() // TestWebSocketFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
