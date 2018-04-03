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

#include "face/tcp-factory.hpp"

#include "face-system-fixture.hpp"
#include "factory-test-common.hpp"
#include "tests/limited-io.hpp"

#include <ndn-cxx/net/address-converter.hpp>

namespace nfd {
namespace face {
namespace tests {

class TcpFactoryFixture : public FaceSystemFactoryFixture<TcpFactory>
{
protected:
  shared_ptr<TcpChannel>
  createChannel(const std::string& localIp, const std::string& localPort)
  {
    tcp::Endpoint endpoint(ndn::ip::addressFromString(localIp),
                           boost::lexical_cast<uint16_t>(localPort));
    return factory.createChannel(endpoint);
  }

protected:
  LimitedIo limitedIo;
};

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpFactory, TcpFactoryFixture)

BOOST_AUTO_TEST_SUITE(ProcessConfig)

BOOST_AUTO_TEST_CASE(Defaults)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"tcp4://0.0.0.0:6363", "tcp6://[::]:6363"});
  auto channels = factory.getChannels();
  BOOST_CHECK(std::all_of(channels.begin(), channels.end(),
                          [] (const shared_ptr<const Channel>& ch) { return ch->isListening(); }));

  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.size(), 2);
  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.count("127.0.0.0/8"), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.count("::1/128"), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.size(), 0);
}

BOOST_AUTO_TEST_CASE(DisableListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        listen no
        port 7001
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"tcp4://0.0.0.0:7001", "tcp6://[::]:7001"});
  auto channels = factory.getChannels();
  BOOST_CHECK(std::none_of(channels.begin(), channels.end(),
                           [] (const shared_ptr<const Channel>& ch) { return ch->isListening(); }));
}

BOOST_AUTO_TEST_CASE(DisableV4)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        port 7001
        enable_v4 no
        enable_v6 yes
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"tcp6://[::]:7001"});
}

BOOST_AUTO_TEST_CASE(DisableV6)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        port 7001
        enable_v4 yes
        enable_v6 no
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  checkChannelListEqual(factory, {"tcp4://0.0.0.0:7001"});
}

BOOST_AUTO_TEST_CASE(ConfigureLocal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        local {
          whitelist {
            subnet 127.0.0.0/8
          }

          blacklist {
            subnet ::1/128
          }
        }
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.size(), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.count("127.0.0.0/8"), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.size(), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.count("::1/128"), 1);

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""},
             [] (const nfd::Face& face) {
               BOOST_CHECK_EQUAL(face.getScope(), ndn::nfd::FACE_SCOPE_LOCAL);
             });

  limitedIo.run(1, 100_ms);
}

BOOST_AUTO_TEST_CASE(ConfigureNonLocal)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        local {
          whitelist {
            *
          }

          blacklist {
            subnet 127.0.0.0/8
            subnet ::1/128
          }
        }
      }
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.size(), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_whitelist.count("*"), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.size(), 2);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.count("127.0.0.0/8"), 1);
  BOOST_CHECK_EQUAL(factory.m_local.m_blacklist.count("::1/128"), 1);

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""},
             [] (const nfd::Face& face) {
               BOOST_CHECK_EQUAL(face.getScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
             });

  limitedIo.run(1, 100_ms);
}

BOOST_AUTO_TEST_CASE(Omitted)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
    }
  )CONFIG";

  parseConfig(CONFIG, true);
  parseConfig(CONFIG, false);

  BOOST_CHECK_EQUAL(factory.getChannels().size(), 0);
}

BOOST_AUTO_TEST_CASE(AllDisabled)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        enable_v4 no
        enable_v6 no
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(BadListen)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        listen hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE_EXPECTED_FAILURES(BadPort, 2) // Bug #4489
BOOST_AUTO_TEST_CASE(BadPort)
{
  // not a number
  const std::string CONFIG1 = R"CONFIG(
    face_system
    {
      tcp
      {
        port hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG1, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG1, false), ConfigFile::Error);

  // negative number
  const std::string CONFIG2 = R"CONFIG(
    face_system
    {
      tcp
      {
        port -1
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG2, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG2, false), ConfigFile::Error);

  // out of range
  const std::string CONFIG3 = R"CONFIG(
    face_system
    {
      tcp
      {
        port 65536
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG3, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG3, false), ConfigFile::Error);
}

BOOST_AUTO_TEST_CASE(UnknownOption)
{
  const std::string CONFIG = R"CONFIG(
    face_system
    {
      tcp
      {
        hello
      }
    }
  )CONFIG";

  BOOST_CHECK_THROW(parseConfig(CONFIG, true), ConfigFile::Error);
  BOOST_CHECK_THROW(parseConfig(CONFIG, false), ConfigFile::Error);
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

BOOST_AUTO_TEST_CASE(CreateChannel)
{
  auto channel1 = createChannel("127.0.0.1", "20070");
  auto channel1a = createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "tcp4://127.0.0.1:20070");

  auto channel2 = createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  auto channel3 = createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel3);
  BOOST_CHECK_EQUAL(channel3->getUri().toString(), "tcp6://[::1]:20071");
}

BOOST_AUTO_TEST_CASE(CreateFace)
{
  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 504, "No channels available to connect"});

  createChannel("127.0.0.1", "20071");

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20072"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERMANENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20073"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, true, false},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20073"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, true},
             {CreateFaceExpectedResult::SUCCESS, 0, ""});
}

BOOST_AUTO_TEST_CASE(UnsupportedCreateFace)
{
  createChannel("127.0.0.1", "20071");

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20072"),
             FaceUri("tcp4://127.0.0.1:20071"),
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Unicast TCP faces cannot be created with a LocalUri"});

  createFace(factory,
             FaceUri("tcp4://127.0.0.1:20072"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_ON_DEMAND, {}, {}, false, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Outgoing TCP faces do not support on-demand persistency"});

  createFace(factory,
             FaceUri("tcp4://198.51.100.100:6363"),
             {},
             {ndn::nfd::FACE_PERSISTENCY_PERSISTENT, {}, {}, true, false, false},
             {CreateFaceExpectedResult::FAILURE, 406,
              "Local fields can only be enabled on faces with local scope"});
}

class CreateFaceTimeoutFixture : public TcpFactoryFixture
{
public:
  void
  onFaceCreated(const shared_ptr<nfd::Face>& newFace)
  {
    BOOST_CHECK_MESSAGE(false, "Timeout expected");
    face = newFace;

    limitedIo.afterOp();
  }

  void
  onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(true, reason);

    limitedIo.afterOp();
  }

public:
  shared_ptr<nfd::Face> face;
};

BOOST_FIXTURE_TEST_CASE(CreateFaceTimeout, CreateFaceTimeoutFixture)
{
  createChannel("0.0.0.0", "20070");
  factory.createFace({FaceUri("tcp4://192.0.2.1:20070"), {}, {}},
                     bind(&CreateFaceTimeoutFixture::onFaceCreated, this, _1),
                     bind(&CreateFaceTimeoutFixture::onConnectFailed, this, _2));

  BOOST_REQUIRE_EQUAL(limitedIo.run(1, 10_s), LimitedIo::EXCEED_OPS);
  BOOST_CHECK(face == nullptr);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpFactory
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
