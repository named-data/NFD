/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "face/tcp-channel.hpp"
#include "face/tcp-face.hpp"
#include "face/tcp-factory.hpp"

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "dummy-stream-sender.hpp"
#include "packet-datasets.hpp"

#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceTcp, BaseFixture)

BOOST_AUTO_TEST_CASE(ChannelMap)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<TcpChannel> channel1a = factory.createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "tcp4://127.0.0.1:20070");

  shared_ptr<TcpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  shared_ptr<TcpChannel> channel3 = factory.createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel3);
  BOOST_CHECK_EQUAL(channel3->getUri().toString(), "tcp6://[::1]:20071");
}

BOOST_AUTO_TEST_CASE(GetChannels)
{
  TcpFactory factory;
  BOOST_REQUIRE_EQUAL(factory.getChannels().empty(), true);

  std::vector<shared_ptr<const Channel>> expectedChannels;
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20070"));
  expectedChannels.push_back(factory.createChannel("127.0.0.1", "20071"));
  expectedChannels.push_back(factory.createChannel("::1", "20071"));

  for (const auto& ch : factory.getChannels()) {
    auto pos = std::find(expectedChannels.begin(), expectedChannels.end(), ch);
    BOOST_REQUIRE(pos != expectedChannels.end());
    expectedChannels.erase(pos);
  }
  BOOST_CHECK_EQUAL(expectedChannels.size(), 0);
}

class FaceCreateFixture : protected BaseFixture
{
public:
  void
  checkError(const std::string& errorActual, const std::string& errorExpected)
  {
    BOOST_CHECK_EQUAL(errorActual, errorExpected);
  }

  void
  failIfError(const std::string& errorActual)
  {
    BOOST_FAIL("No error expected, but got: [" << errorActual << "]");
  }
};

BOOST_FIXTURE_TEST_CASE(FaceCreate, FaceCreateFixture)
{
  TcpFactory factory;

  factory.createFace(FaceUri("tcp4://127.0.0.1:6363"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::checkError, this, _1,
                          "No channels available to connect to 127.0.0.1:6363"));

  factory.createChannel("127.0.0.1", "20071");
  
  factory.createFace(FaceUri("tcp4://127.0.0.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));
}

BOOST_FIXTURE_TEST_CASE(UnsupportedFaceCreate, FaceCreateFixture)
{
  TcpFactory factory;

  factory.createChannel("127.0.0.1", "20070");
  factory.createChannel("127.0.0.1", "20071");

  BOOST_CHECK_THROW(factory.createFace(FaceUri("tcp4://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("tcp4://127.0.0.1:20071"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

class EndToEndFixture : protected BaseFixture
{
public:
  void
  channel1_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = newFace;
    face1->onReceiveInterest.connect(bind(&EndToEndFixture::face1_onReceiveInterest, this, _1));
    face1->onReceiveData.connect(bind(&EndToEndFixture::face1_onReceiveData, this, _1));
    face1->onFail.connect(bind(&EndToEndFixture::face1_onFail, this));

    limitedIo.afterOp();
  }

  void
  channel1_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    limitedIo.afterOp();
  }

  void
  face1_onReceiveInterest(const Interest& interest)
  {
    face1_receivedInterests.push_back(interest);

    limitedIo.afterOp();
  }

  void
  face1_onReceiveData(const Data& data)
  {
    face1_receivedDatas.push_back(data);

    limitedIo.afterOp();
  }

  void
  face1_onFail()
  {
    face1.reset();
    limitedIo.afterOp();
  }

  void
  channel2_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face2));
    face2 = newFace;
    face2->onReceiveInterest.connect(bind(&EndToEndFixture::face2_onReceiveInterest, this, _1));
    face2->onReceiveData.connect(bind(&EndToEndFixture::face2_onReceiveData, this, _1));
    face2->onFail.connect(bind(&EndToEndFixture::face2_onFail, this));

    limitedIo.afterOp();
  }

  void
  channel2_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    limitedIo.afterOp();
  }

  void
  face2_onReceiveInterest(const Interest& interest)
  {
    face2_receivedInterests.push_back(interest);

    limitedIo.afterOp();
  }

  void
  face2_onReceiveData(const Data& data)
  {
    face2_receivedDatas.push_back(data);

    limitedIo.afterOp();
  }

  void
  face2_onFail()
  {
    face2.reset();
    limitedIo.afterOp();
  }

  void
  channel_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    faces.push_back(newFace);
    limitedIo.afterOp();
  }

  void
  channel_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    limitedIo.afterOp();
  }

  void
  checkFaceList(size_t shouldBe)
  {
    BOOST_CHECK_EQUAL(faces.size(), shouldBe);
  }

  void
  connect(const shared_ptr<TcpChannel>& channel,
          const std::string& remoteHost,
          const std::string& remotePort)
  {
    channel->connect(tcp::Endpoint(boost::asio::ip::address::from_string(remoteHost),
                                   boost::lexical_cast<uint16_t>(remotePort)),
                     bind(&EndToEndFixture::channel_onFaceCreated, this, _1),
                     bind(&EndToEndFixture::channel_onConnectFailed, this, _1));
  }

public:
  LimitedIo limitedIo;

  shared_ptr<Face> face1;
  std::vector<Interest> face1_receivedInterests;
  std::vector<Data> face1_receivedDatas;
  shared_ptr<Face> face2;
  std::vector<Interest> face2_receivedInterests;
  std::vector<Data> face2_receivedDatas;

  std::list<shared_ptr<Face>> faces;
};

BOOST_FIXTURE_TEST_CASE(EndToEnd4, EndToEndFixture)
{
  TcpFactory factory1;

  shared_ptr<TcpChannel> channel1 = factory1.createChannel("127.0.0.1", "20070");
  factory1.createChannel("127.0.0.1", "20071");

  BOOST_CHECK_EQUAL(channel1->isListening(), false);

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  BOOST_CHECK_EQUAL(channel1->isListening(), true);

  TcpFactory factory2;

  shared_ptr<TcpChannel> channel2 = factory2.createChannel("127.0.0.2", "20070");
  factory2.createChannel("127.0.0.2", "20071");

  factory2.createFace(FaceUri("tcp4://127.0.0.1:20070"),
                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                      bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                      bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_REQUIRE(static_cast<bool>(face2));

  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face2->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(face1->isMultiAccess(), false);
  BOOST_CHECK_EQUAL(face2->isMultiAccess(), false);

  BOOST_CHECK_EQUAL(face2->getRemoteUri().toString(), "tcp4://127.0.0.1:20070");
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "tcp4://127.0.0.1:20070");
  // face1 has an unknown remoteUri, since the source port is automatically chosen by OS

  BOOST_CHECK_EQUAL(face1->isLocal(), true);
  BOOST_CHECK_EQUAL(face2->isLocal(), true);

  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face1)), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face2)), true);

  // integrated tests needs to check that TcpFace for non-loopback fails these tests...

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  face1->sendInterest(*interest1);
  face1->sendInterest(*interest1);
  face1->sendInterest(*interest1);
  face1->sendData    (*data1    );
  size_t nBytesSent1 = interest1->wireEncode().size() * 3 + data1->wireEncode().size();
  face2->sendInterest(*interest2);
  face2->sendData    (*data2    );
  face2->sendData    (*data2    );
  face2->sendData    (*data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(8, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2->getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2->getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1->getName());

  // needed to ensure NOutBytes counters are accurate
  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::seconds(1));

  const FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.getNInInterests() , 1);
  BOOST_CHECK_EQUAL(counters1.getNInDatas()     , 3);
  BOOST_CHECK_EQUAL(counters1.getNOutInterests(), 3);
  BOOST_CHECK_EQUAL(counters1.getNOutDatas()    , 1);
  BOOST_CHECK_EQUAL(counters1.getNOutBytes(), nBytesSent1);

  const FaceCounters& counters2 = face2->getCounters();
  BOOST_CHECK_EQUAL(counters2.getNInInterests() , 3);
  BOOST_CHECK_EQUAL(counters2.getNInDatas()     , 1);
  BOOST_CHECK_EQUAL(counters2.getNOutInterests(), 1);
  BOOST_CHECK_EQUAL(counters2.getNOutDatas()    , 3);
  BOOST_CHECK_EQUAL(counters2.getNInBytes(), nBytesSent1);
}

BOOST_FIXTURE_TEST_CASE(EndToEnd6, EndToEndFixture)
{
  TcpFactory factory1;

  shared_ptr<TcpChannel> channel1 = factory1.createChannel("::1", "20070");
  shared_ptr<TcpChannel> channel2 = factory1.createChannel("::1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  TcpFactory factory2;

  factory2.createChannel("::2", "20070");

  factory2.createFace(FaceUri("tcp6://[::1]:20070"),
                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                      bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                      bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_REQUIRE(static_cast<bool>(face2));

  BOOST_CHECK_EQUAL(face2->getRemoteUri().toString(), "tcp6://[::1]:20070");
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "tcp6://[::1]:20070");
  // face1 has an unknown remoteUri, since the source port is automatically chosen by OS

  BOOST_CHECK_EQUAL(face1->isLocal(), true);
  BOOST_CHECK_EQUAL(face2->isLocal(), true);

  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face1)), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face2)), true);

  // integrated tests needs to check that TcpFace for non-loopback fails these tests...

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  face1->sendInterest(*interest1);
  face1->sendData    (*data1    );
  face2->sendInterest(*interest2);
  face2->sendData    (*data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(4, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot send or receive Interest/Data packets");


  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2->getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2->getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1->getName());
}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<TcpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  using namespace boost::asio;
  channel2->connect(tcp::Endpoint(ip::address::from_string("127.0.0.1"), 20070),
                    bind(&EndToEndFixture::channel_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1),
                    time::seconds(4)); // very short timeout

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");


  BOOST_CHECK_EQUAL(faces.size(), 2);

  shared_ptr<TcpChannel> channel3 = factory.createChannel("127.0.0.1", "20072");
  channel3->connect(tcp::Endpoint(ip::address::from_string("127.0.0.1"), 20070),
                    bind(&EndToEndFixture::channel_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1),
                    time::seconds(4)); // very short timeout


  shared_ptr<TcpChannel> channel4 = factory.createChannel("127.0.0.1", "20073");

  BOOST_CHECK_NE(channel3, channel4);

  scheduler::schedule(time::seconds(1),
                      bind(&EndToEndFixture::connect, this, channel4, "127.0.0.1", "20070"));

  scheduler::schedule(time::milliseconds(500),
                      bind(&EndToEndFixture::checkFaceList, this, 4));

  BOOST_CHECK_MESSAGE(limitedIo.run(4,// 2 connects and 2 accepts
                      time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept multiple connections");

  BOOST_CHECK_EQUAL(faces.size(), 6);
}


BOOST_FIXTURE_TEST_CASE(FaceClosing, EndToEndFixture)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<TcpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  using namespace boost::asio;
  channel2->connect(tcp::Endpoint(ip::address::from_string("127.0.0.1"), 20070),
                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel2_onConnectFailed, this, _1),
                    time::seconds(4)); // very short timeout

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_CHECK_EQUAL(channel2->size(), 1);

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK(static_cast<bool>(face2));

  // Face::close must be invoked during io run to be counted as an op
  scheduler::schedule(time::milliseconds(100), bind(&Face::close, face1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "FaceClosing error: cannot properly close faces");

  // both faces should get closed
  BOOST_CHECK(!static_cast<bool>(face1));
  BOOST_CHECK(!static_cast<bool>(face2));

  BOOST_CHECK_EQUAL(channel1->size(), 0);
  BOOST_CHECK_EQUAL(channel2->size(), 0);
}


class SimpleEndToEndFixture : protected BaseFixture
{
public:
  void
  onFaceCreated(const shared_ptr<Face>& face)
  {
    face->onReceiveInterest.connect(bind(&SimpleEndToEndFixture::onReceiveInterest, this, _1));
    face->onReceiveData.connect(bind(&SimpleEndToEndFixture::onReceiveData, this, _1));
    face->onFail.connect(bind(&SimpleEndToEndFixture::onFail, this, face));

    if (static_cast<bool>(dynamic_pointer_cast<LocalFace>(face))) {
      static_pointer_cast<LocalFace>(face)->setLocalControlHeaderFeature(
        LOCAL_CONTROL_FEATURE_INCOMING_FACE_ID);

      static_pointer_cast<LocalFace>(face)->setLocalControlHeaderFeature(
        LOCAL_CONTROL_FEATURE_NEXT_HOP_FACE_ID);
    }

    limitedIo.afterOp();
  }

  void
  onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    limitedIo.afterOp();
  }

  void
  onReceiveInterest(const Interest& interest)
  {
    receivedInterests.push_back(interest);

    limitedIo.afterOp();
  }

  void
  onReceiveData(const Data& data)
  {
    receivedDatas.push_back(data);

    limitedIo.afterOp();
  }

  void
  onFail(const shared_ptr<Face>& face)
  {
    limitedIo.afterOp();
  }

public:
  LimitedIo limitedIo;

  std::vector<Interest> receivedInterests;
  std::vector<Data> receivedDatas;
};


BOOST_FIXTURE_TEST_CASE_TEMPLATE(LocalFaceCorruptedInput, Dataset,
                                 CorruptedPackets, SimpleEndToEndFixture)
{
  TcpFactory factory;
  tcp::Endpoint endpoint(boost::asio::ip::address_v4::from_string("127.0.0.1"), 20070);

  shared_ptr<TcpChannel> channel = factory.createChannel(endpoint);
  channel->listen(bind(&SimpleEndToEndFixture::onFaceCreated,   this, _1),
                  bind(&SimpleEndToEndFixture::onConnectFailed, this, _1));
  BOOST_REQUIRE_EQUAL(channel->isListening(), true);

  DummyStreamSender<boost::asio::ip::tcp, Dataset> sender;
  sender.start(endpoint);

  BOOST_CHECK_MESSAGE(limitedIo.run(LimitedIo::UNLIMITED_OPS,
                                    time::seconds(1)) == LimitedIo::EXCEED_TIME,
                      "Exception thrown for " + Dataset::getName());
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(FaceCorruptedInput, Dataset,
                                 CorruptedPackets, SimpleEndToEndFixture)
{
  // test with non-local Face
  boost::asio::ip::address_v4 someIpv4Address;
  for (const auto& netif : listNetworkInterfaces()) {
    if (!netif.isLoopback() && netif.isUp() && !netif.ipv4Addresses.empty()) {
      someIpv4Address = netif.ipv4Addresses.front();
      break;
    }
  }
  if (someIpv4Address.is_unspecified()) {
    BOOST_TEST_MESSAGE("Test with non-local Face cannot be run "
                       "(no non-loopback interface with IPv4 address found)");
    return;
  }

  TcpFactory factory;
  tcp::Endpoint endpoint(someIpv4Address, 20070);

  shared_ptr<TcpChannel> channel = factory.createChannel(endpoint);
  channel->listen(bind(&SimpleEndToEndFixture::onFaceCreated,   this, _1),
                  bind(&SimpleEndToEndFixture::onConnectFailed, this, _1));
  BOOST_REQUIRE_EQUAL(channel->isListening(), true);

  DummyStreamSender<boost::asio::ip::tcp, Dataset> sender;
  sender.start(endpoint);

  BOOST_CHECK_MESSAGE(limitedIo.run(LimitedIo::UNLIMITED_OPS,
                                    time::seconds(1)) == LimitedIo::EXCEED_TIME,
                      "Exception thrown for " + Dataset::getName());
}

class FaceCreateTimeoutFixture : protected BaseFixture
{
public:
  void
  onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK_MESSAGE(false, "Timeout expected");
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = newFace;

    limitedIo.afterOp();
  }

  void
  onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(true, reason);

    limitedIo.afterOp();
  }

public:
  LimitedIo limitedIo;

  shared_ptr<Face> face1;
};

BOOST_FIXTURE_TEST_CASE(FaceCreateTimeout, FaceCreateTimeoutFixture)
{
  TcpFactory factory;
  shared_ptr<TcpChannel> channel = factory.createChannel("0.0.0.0", "20070");

  factory.createFace(FaceUri("tcp4://192.0.2.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind(&FaceCreateTimeoutFixture::onFaceCreated, this, _1),
                     bind(&FaceCreateTimeoutFixture::onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(static_cast<bool>(face1), false);
}

BOOST_FIXTURE_TEST_CASE(Bug1856, EndToEndFixture)
{
  TcpFactory factory1;

  shared_ptr<TcpChannel> channel1 = factory1.createChannel("127.0.0.1", "20070");
  factory1.createChannel("127.0.0.1", "20071");

  BOOST_CHECK_EQUAL(channel1->isListening(), false);

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  BOOST_CHECK_EQUAL(channel1->isListening(), true);

  TcpFactory factory2;

  shared_ptr<TcpChannel> channel2 = factory2.createChannel("127.0.0.2", "20070");
  factory2.createChannel("127.0.0.2", "20071");

  factory2.createFace(FaceUri("tcp4://127.0.0.1:20070"),
                      ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                      bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                      bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_REQUIRE(static_cast<bool>(face2));

  std::ostringstream hugeName;
  hugeName << "/huge-name/";
  for (size_t i = 0; i < ndn::MAX_NDN_PACKET_SIZE; i++)
    hugeName << 'a';

  shared_ptr<Interest> interest = makeInterest("ndn:/KfczhUqVix");
  shared_ptr<Interest> hugeInterest = makeInterest(hugeName.str());

  face1->sendInterest(*hugeInterest);
  face2->sendInterest(*interest);
  face2->sendInterest(*interest);

  limitedIo.run(LimitedIo::UNLIMITED_OPS, time::seconds(1));
  BOOST_TEST_MESSAGE("Unexpected assertion test passed");
}

class FakeNetworkInterfaceFixture : public BaseFixture
{
public:
  FakeNetworkInterfaceFixture()
  {
    using namespace boost::asio::ip;

    auto fakeInterfaces = make_shared<std::vector<NetworkInterfaceInfo>>();

    fakeInterfaces->push_back(
      NetworkInterfaceInfo {0, "eth0",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("0.0.0.0")},
        {address_v6::from_string("::")},
        address_v4(),
        IFF_UP});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {1, "eth0",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("192.168.2.1"), address_v4::from_string("192.168.2.2")},
        {},
        address_v4::from_string("192.168.2.255"),
        0});
    fakeInterfaces->push_back(
      NetworkInterfaceInfo {2, "eth1",
        ethernet::Address::fromString("3e:15:c2:8b:65:00"),
        {address_v4::from_string("198.51.100.1")},
        {address_v6::from_string("2001:db8::2"), address_v6::from_string("2001:db8::3")},
        address_v4::from_string("198.51.100.255"),
        IFF_MULTICAST | IFF_BROADCAST | IFF_UP});

    setDebugNetworkInterfaces(fakeInterfaces);
  }

  ~FakeNetworkInterfaceFixture()
  {
    setDebugNetworkInterfaces(nullptr);
  }
};

BOOST_FIXTURE_TEST_CASE(Bug2292, FakeNetworkInterfaceFixture)
{
  using namespace boost::asio::ip;

  TcpFactory factory;
  factory.prohibitEndpoint(tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v6::from_string("2001:db8::1"), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v6::from_string("2001:db8::1"), 2048)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v4(), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 4);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
                 tcp::Endpoint(address_v4::from_string("192.168.2.2"), 1024),
                 tcp::Endpoint(address_v4::from_string("198.51.100.1"), 1024),
                 tcp::Endpoint(address_v4::from_string("0.0.0.0"), 1024)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(tcp::Endpoint(address_v6(), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 3);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<tcp::Endpoint> {
                 tcp::Endpoint(address_v6::from_string("2001:db8::2"), 2048),
                 tcp::Endpoint(address_v6::from_string("2001:db8::3"), 2048),
                 tcp::Endpoint(address_v6::from_string("::"), 2048)
               }));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
