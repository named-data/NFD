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

#include "face/unix-stream-channel.hpp"
#include "face/unix-stream-factory.hpp"
#include "face/unix-stream-transport.hpp"

#include "face/generic-link-service.hpp"
#include "face/lp-face-wrapper.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "dummy-stream-sender.hpp"
#include "packet-datasets.hpp"

namespace nfd {
namespace tests {

#define CHANNEL_PATH1 "unix-stream-test.1.sock"
#define CHANNEL_PATH2 "unix-stream-test.2.sock"

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestUnixStream, BaseFixture)

using nfd::Face;
using face::LpFaceWrapper;
using face::UnixStreamTransport;

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

  BOOST_CHECK_THROW(factory.createFace(FaceUri("unix:///var/run/nfd.sock"),
                                       ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("unix:///var/run/nfd.sock"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("unix:///var/run/nfd.sock"),
                                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

class EndToEndFixture : protected BaseFixture
{
public:
  void
  client_onConnect(const boost::system::error_code& error)
  {
    BOOST_CHECK_MESSAGE(!error, error.message());

    limitedIo.afterOp();
  }

  void
  channel1_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = static_pointer_cast<LpFaceWrapper>(newFace);
    face1->onReceiveInterest.connect(bind(&EndToEndFixture::face1_onReceiveInterest, this, _1));
    face1->onReceiveData.connect(bind(&EndToEndFixture::face1_onReceiveData, this, _1));

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
  channel_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    faces.push_back(static_pointer_cast<LpFaceWrapper>(newFace));

    limitedIo.afterOp();
  }

  void
  channel_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    limitedIo.afterOp();
  }

  shared_ptr<LpFaceWrapper>
  makeFace(UnixStreamTransport::protocol::socket&& socket)
  {
    auto linkService = make_unique<face::GenericLinkService>();
    auto transport = make_unique<UnixStreamTransport>(std::move(socket));
    auto lpFace = make_unique<LpFace>(std::move(linkService), std::move(transport));
    return make_shared<LpFaceWrapper>(std::move(lpFace));
  }

protected:
  LimitedIo limitedIo;

  shared_ptr<LpFaceWrapper> face1;
  std::vector<Interest> face1_receivedInterests;
  std::vector<Data> face1_receivedDatas;
  shared_ptr<LpFaceWrapper> face2;
  std::vector<Interest> face2_receivedInterests;
  std::vector<Data> face2_receivedDatas;

  std::list<shared_ptr<LpFaceWrapper>> faces;
};


BOOST_FIXTURE_TEST_CASE(EndToEnd, EndToEndFixture)
{
  UnixStreamFactory factory;

  shared_ptr<UnixStreamChannel> channel1 = factory.createChannel(CHANNEL_PATH1);
  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  UnixStreamTransport::protocol::socket client(g_io);
  client.async_connect(UnixStreamTransport::protocol::endpoint(CHANNEL_PATH1),
                       bind(&EndToEndFixture::client_onConnect, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS, "Connect");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK_EQUAL(face1->isLocal(), true);
  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face1->isMultiAccess(), false);
  BOOST_CHECK_EQUAL(face1->getRemoteUri().getScheme(), "fd");
  BOOST_CHECK_NO_THROW(std::stoi(face1->getRemoteUri().getHost()));
  std::string face1localUri = face1->getLocalUri().toString();
  BOOST_CHECK_EQUAL(face1localUri.find("unix:///"), 0); // third '/' is the path separator
  BOOST_CHECK_EQUAL(face1localUri.rfind(CHANNEL_PATH1),
                    face1localUri.size() - std::string(CHANNEL_PATH1).size());

  face2 = makeFace(std::move(client));
  face2->onReceiveInterest.connect(bind(&EndToEndFixture::face2_onReceiveInterest, this, _1));
  face2->onReceiveData.connect(bind(&EndToEndFixture::face2_onReceiveData, this, _1));

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
  size_t nBytesSent2 = interest2->wireEncode().size() + data2->wireEncode().size() * 3;

  BOOST_CHECK_MESSAGE(limitedIo.run(8, time::seconds(1)) == LimitedIo::EXCEED_OPS, "Send/receive");

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

  const face::FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.nInInterests, 1);
  BOOST_CHECK_EQUAL(counters1.nInData, 3);
  BOOST_CHECK_EQUAL(counters1.nOutInterests, 3);
  BOOST_CHECK_EQUAL(counters1.nOutData, 1);
  BOOST_CHECK_EQUAL(counters1.nInBytes, nBytesSent2);
  BOOST_CHECK_EQUAL(counters1.nOutBytes, nBytesSent1);

  const face::FaceCounters& counters2 = face2->getCounters();
  BOOST_CHECK_EQUAL(counters2.nInInterests, 3);
  BOOST_CHECK_EQUAL(counters2.nInData, 1);
  BOOST_CHECK_EQUAL(counters2.nOutInterests, 1);
  BOOST_CHECK_EQUAL(counters2.nOutData, 3);
}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  UnixStreamFactory factory;

  shared_ptr<UnixStreamChannel> channel = factory.createChannel(CHANNEL_PATH1);
  channel->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                  bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  UnixStreamTransport::protocol::socket client1(g_io);
  client1.async_connect(UnixStreamTransport::protocol::endpoint(CHANNEL_PATH1),
                        bind(&EndToEndFixture::client_onConnect, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS, "First connect");

  BOOST_CHECK_EQUAL(faces.size(), 1);

  UnixStreamTransport::protocol::socket client2(g_io);
  client2.async_connect(UnixStreamTransport::protocol::endpoint(CHANNEL_PATH1),
                        bind(&EndToEndFixture::client_onConnect, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS, "Second connect");

  BOOST_CHECK_EQUAL(faces.size(), 2);

  // now close one of the faces
  faces.front()->close();

  // we should still be able to send/receive with the other one
  face1 = faces.back();
  face1->onReceiveInterest.connect(bind(&EndToEndFixture::face1_onReceiveInterest, this, _1));
  face1->onReceiveData.connect(bind(&EndToEndFixture::face1_onReceiveData, this, _1));

  face2 = makeFace(std::move(client2));
  face2->onReceiveInterest.connect(bind(&EndToEndFixture::face2_onReceiveInterest, this, _1));
  face2->onReceiveData.connect(bind(&EndToEndFixture::face2_onReceiveData, this, _1));

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  face1->sendInterest(*interest1);
  face1->sendData    (*data1    );
  face2->sendInterest(*interest2);
  face2->sendData    (*data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(4, time::seconds(1)) == LimitedIo::EXCEED_OPS, "Send/receive");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2->getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2->getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1->getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1->getName());
}

BOOST_AUTO_TEST_SUITE_END() // TestUnixStream
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
