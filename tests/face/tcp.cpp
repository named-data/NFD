/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/tcp-factory.hpp"
#include <ndn-cpp-dev/security/key-chain.hpp>

#include "tests/test-common.hpp"
#include "tests/core/limited-io.hpp"

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

class EndToEndFixture : protected BaseFixture
{
public:
  void
  channel1_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    face1 = newFace;
    face1->onReceiveInterest +=
      bind(&EndToEndFixture::face1_onReceiveInterest, this, _1);
    face1->onReceiveData +=
      bind(&EndToEndFixture::face1_onReceiveData, this, _1);
    face1->onFail +=
      bind(&EndToEndFixture::face1_onFail, this);

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
    face2->onReceiveInterest +=
      bind(&EndToEndFixture::face2_onReceiveInterest, this, _1);
    face2->onReceiveData +=
      bind(&EndToEndFixture::face2_onReceiveData, this, _1);
    face2->onFail +=
      bind(&EndToEndFixture::face2_onFail, this);

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

public:
  LimitedIo limitedIo;

  shared_ptr<Face> face1;
  std::vector<Interest> face1_receivedInterests;
  std::vector<Data> face1_receivedDatas;
  shared_ptr<Face> face2;
  std::vector<Interest> face2_receivedInterests;
  std::vector<Data> face2_receivedDatas;

  std::list< shared_ptr<Face> > faces;
};


BOOST_FIXTURE_TEST_CASE(EndToEnd4, EndToEndFixture)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  factory.createChannel("127.0.0.1", "20071");

  BOOST_CHECK_EQUAL(channel1->isListening(), false);

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  BOOST_CHECK_EQUAL(channel1->isListening(), true);

  factory.createFace(FaceUri("tcp://127.0.0.1:20070"),
                     bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                     bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_REQUIRE(static_cast<bool>(face2));

  BOOST_CHECK(face1->isOnDemand());
  BOOST_CHECK(!face2->isOnDemand());

  BOOST_CHECK_EQUAL(face2->getRemoteUri().toString(), "tcp4://127.0.0.1:20070");
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "tcp4://127.0.0.1:20070");
  // face1 has an unknown remoteUri, since the source port is automatically chosen by OS

  BOOST_CHECK_EQUAL(face1->isLocal(), true);
  BOOST_CHECK_EQUAL(face2->isLocal(), true);

  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face1)), true);
  BOOST_CHECK_EQUAL(static_cast<bool>(dynamic_pointer_cast<LocalFace>(face2)), true);

  // integrated tests needs to check that TcpFace for non-loopback fails these tests...

  Interest interest1("ndn:/TpnzGvW9R");
  Data     data1    ("ndn:/KfczhUqVix");
  data1.setContent(0, 0);
  Interest interest2("ndn:/QWiIMfj5sL");
  Data     data2    ("ndn:/XNBV796f");
  data2.setContent(0, 0);

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  // set fake signature on data1 and data2
  data1.setSignature(fakeSignature);
  data2.setSignature(fakeSignature);

  face1->sendInterest(interest1);
  face1->sendInterest(interest1);
  face1->sendInterest(interest1);
  face1->sendData    (data1    );
  face2->sendInterest(interest2);
  face2->sendData    (data2    );
  face2->sendData    (data2    );
  face2->sendData    (data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(8, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot send or receive Interest/Data packets");


  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1.getName());

  const FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.getNInInterests() , 1);
  BOOST_CHECK_EQUAL(counters1.getNInDatas()     , 3);
  BOOST_CHECK_EQUAL(counters1.getNOutInterests(), 3);
  BOOST_CHECK_EQUAL(counters1.getNOutDatas()    , 1);

  const FaceCounters& counters2 = face2->getCounters();
  BOOST_CHECK_EQUAL(counters2.getNInInterests() , 3);
  BOOST_CHECK_EQUAL(counters2.getNInDatas()     , 1);
  BOOST_CHECK_EQUAL(counters2.getNOutInterests(), 1);
  BOOST_CHECK_EQUAL(counters2.getNOutDatas()    , 3);
}

BOOST_FIXTURE_TEST_CASE(EndToEnd6, EndToEndFixture)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("::1", "20070");
  shared_ptr<TcpChannel> channel2 = factory.createChannel("::1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  factory.createFace(FaceUri("tcp://[::1]:20070"),
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

  Interest interest1("ndn:/TpnzGvW9R");
  Data     data1    ("ndn:/KfczhUqVix");
  data1.setContent(0, 0);
  Interest interest2("ndn:/QWiIMfj5sL");
  Data     data2    ("ndn:/XNBV796f");
  data2.setContent(0, 0);

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));

  // set fake signature on data1 and data2
  data1.setSignature(fakeSignature);
  data2.setSignature(fakeSignature);

  face1->sendInterest(interest1);
  face1->sendData    (data1    );
  face2->sendInterest(interest2);
  face2->sendData    (data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(4, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot send or receive Interest/Data packets");


  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1.getName());
}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  TcpFactory factory;

  shared_ptr<TcpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<TcpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  channel2->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1),
                    time::seconds(4)); // very short timeout

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(10)) == LimitedIo::EXCEED_OPS,
                      "TcpChannel error: cannot connect or cannot accept connection");


  BOOST_CHECK_EQUAL(faces.size(), 2);

  shared_ptr<TcpChannel> channel3 = factory.createChannel("127.0.0.1", "20072");
  channel3->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1),
                    time::seconds(4)); // very short timeout


  shared_ptr<TcpChannel> channel4 = factory.createChannel("127.0.0.1", "20073");

  BOOST_CHECK_NE(channel3, channel4);

  scheduler::schedule(time::seconds(1),
      bind(&TcpChannel::connect, channel4, "127.0.0.1", "20070",
          // does not work without static_cast
           static_cast<TcpChannel::FaceCreatedCallback>(
               bind(&EndToEndFixture::channel_onFaceCreated, this, _1)),
           static_cast<TcpChannel::ConnectFailedCallback>(
               bind(&EndToEndFixture::channel_onConnectFailed, this, _1)),
           time::seconds(4)));

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

  channel2->connect("127.0.0.1", "20070",
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


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
