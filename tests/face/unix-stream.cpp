/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/unix-stream-channel-factory.hpp"
#include "core/scheduler.hpp"

#include <ndn-cpp-dev/security/key-chain.hpp>

#include <boost/test/unit_test.hpp>

using namespace boost::asio::local;

namespace nfd {

BOOST_AUTO_TEST_SUITE(FaceUnixStream)

BOOST_AUTO_TEST_CASE(ChannelMap)
{
  boost::asio::io_service io;
  UnixStreamChannelFactory factory(io);

  shared_ptr<UnixStreamChannel> channel1 = factory.create("foo");
  shared_ptr<UnixStreamChannel> channel1a = factory.create("foo");
  BOOST_CHECK_EQUAL(channel1, channel1a);

  shared_ptr<UnixStreamChannel> channel2 = factory.create("bar");
  BOOST_CHECK_NE(channel1, channel2);
}

class EndToEndFixture
{
public:
  void
  client_onConnect(const boost::system::error_code& error)
  {
    BOOST_CHECK_MESSAGE(!error, error.message());

    this->afterIo();
  }

  void
  channel1_onFaceCreated(const shared_ptr<UnixStreamFace>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(m_face1));
    m_face1 = newFace;
    m_face1->onReceiveInterest +=
      bind(&EndToEndFixture::face1_onReceiveInterest, this, _1);
    m_face1->onReceiveData +=
      bind(&EndToEndFixture::face1_onReceiveData, this, _1);

    this->afterIo();
  }

  void
  channel1_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    this->afterIo();
  }

  void
  face1_onReceiveInterest(const Interest& interest)
  {
    m_face1_receivedInterests.push_back(interest);

    this->afterIo();
  }

  void
  face1_onReceiveData(const Data& data)
  {
    m_face1_receivedDatas.push_back(data);

    this->afterIo();
  }

  void
  face2_onReceiveInterest(const Interest& interest)
  {
    m_face2_receivedInterests.push_back(interest);

    this->afterIo();
  }

  void
  face2_onReceiveData(const Data& data)
  {
    m_face2_receivedDatas.push_back(data);

    this->afterIo();
  }

  void
  channel_onFaceCreated(const shared_ptr<UnixStreamFace>& newFace)
  {
    m_faces.push_back(newFace);

    this->afterIo();
  }

  void
  channel_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    this->afterIo();
  }

  void
  abortTestCase(const std::string& message)
  {
    m_ioService.stop();
    BOOST_FAIL(message);
  }

private:
  void
  afterIo()
  {
    if (--m_ioRemaining <= 0)
      m_ioService.stop();
  }

protected:
  boost::asio::io_service m_ioService;

  int m_ioRemaining;

  shared_ptr<UnixStreamFace> m_face1;
  std::vector<Interest> m_face1_receivedInterests;
  std::vector<Data> m_face1_receivedDatas;
  shared_ptr<UnixStreamFace> m_face2;
  std::vector<Interest> m_face2_receivedInterests;
  std::vector<Data> m_face2_receivedDatas;

  std::list< shared_ptr<UnixStreamFace> > m_faces;
};


BOOST_FIXTURE_TEST_CASE(EndToEnd, EndToEndFixture)
{
  UnixStreamChannelFactory factory(m_ioService);
  Scheduler scheduler(m_ioService); // to limit the amount of time the test may take

  EventId abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot connect or cannot accept connection"));

  shared_ptr<UnixStreamChannel> channel1 = factory.create("foo");
  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  shared_ptr<stream_protocol::socket> client =
      make_shared<stream_protocol::socket>(boost::ref(m_ioService));
  client->async_connect(stream_protocol::endpoint("foo"),
                        bind(&EndToEndFixture::client_onConnect, this, _1));

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();
  scheduler.cancelEvent(abortEvent);

  BOOST_REQUIRE(static_cast<bool>(m_face1));

  abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot send or receive Interest/Data packets"));

  m_face2 = make_shared<UnixStreamFace>(client);
  m_face2->onReceiveInterest +=
    bind(&EndToEndFixture::face2_onReceiveInterest, this, _1);
  m_face2->onReceiveData +=
    bind(&EndToEndFixture::face2_onReceiveData, this, _1);

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

  m_face1->sendInterest(interest1);
  m_face1->sendData    (data1    );
  m_face2->sendInterest(interest2);
  m_face2->sendData    (data2    );

  m_ioRemaining = 4;
  m_ioService.run();
  m_ioService.reset();

  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedDatas    [0].getName(), data1.getName());
}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  UnixStreamChannelFactory factory(m_ioService);
  Scheduler scheduler(m_ioService); // to limit the amount of time the test may take

  EventId abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot connect or cannot accept connection"));

  shared_ptr<UnixStreamChannel> channel = factory.create("foo");
  channel->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                  bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  shared_ptr<stream_protocol::socket> client1 =
      make_shared<stream_protocol::socket>(boost::ref(m_ioService));
  client1->async_connect(stream_protocol::endpoint("foo"),
                         bind(&EndToEndFixture::client_onConnect, this, _1));

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();
  scheduler.cancelEvent(abortEvent);

  BOOST_CHECK_EQUAL(m_faces.size(), 1);

  abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot accept multiple connections"));

  shared_ptr<stream_protocol::socket> client2 =
      make_shared<stream_protocol::socket>(boost::ref(m_ioService));
  client2->async_connect(stream_protocol::endpoint("foo"),
                         bind(&EndToEndFixture::client_onConnect, this, _1));

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();
  scheduler.cancelEvent(abortEvent);

  BOOST_CHECK_EQUAL(m_faces.size(), 2);

  // now close one of the faces
  m_faces.front()->close();

  // we should still be able to send/receive with the other one
  m_face1 = m_faces.back();
  m_face1->onReceiveInterest +=
      bind(&EndToEndFixture::face1_onReceiveInterest, this, _1);
  m_face1->onReceiveData +=
      bind(&EndToEndFixture::face1_onReceiveData, this, _1);

  m_face2 = make_shared<UnixStreamFace>(client2);
  m_face2->onReceiveInterest +=
      bind(&EndToEndFixture::face2_onReceiveInterest, this, _1);
  m_face2->onReceiveData +=
      bind(&EndToEndFixture::face2_onReceiveData, this, _1);

  abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot send or receive Interest/Data packets"));

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

  m_face1->sendInterest(interest1);
  m_face1->sendData    (data1    );
  m_face2->sendInterest(interest2);
  m_face2->sendData    (data2    );

  m_ioRemaining = 4;
  m_ioService.run();
  m_ioService.reset();

  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedDatas    [0].getName(), data1.getName());
}

static inline void
noOp()
{
}

BOOST_FIXTURE_TEST_CASE(UnixStreamFaceLocalControlHeader, EndToEndFixture)
{
  UnixStreamChannelFactory factory(m_ioService);
  Scheduler scheduler(m_ioService); // to limit the amount of time the test may take

  EventId abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot connect or cannot accept connection"));

  shared_ptr<UnixStreamChannel> channel1 = factory.create("foo");
  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  shared_ptr<stream_protocol::socket> client =
      make_shared<stream_protocol::socket>(boost::ref(m_ioService));
  client->async_connect(stream_protocol::endpoint("foo"),
                        bind(&EndToEndFixture::client_onConnect, this, _1));

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();
  scheduler.cancelEvent(abortEvent);

  BOOST_REQUIRE(static_cast<bool>(m_face1));

  abortEvent =
    scheduler.scheduleEvent(time::seconds(1),
                            bind(&EndToEndFixture::abortTestCase, this,
                                 "UnixStreamChannel error: cannot send or receive Interest/Data packets"));

  m_face2 = make_shared<UnixStreamFace>(client);
  m_face2->onReceiveInterest +=
    bind(&EndToEndFixture::face2_onReceiveInterest, this, _1);
  m_face2->onReceiveData +=
    bind(&EndToEndFixture::face2_onReceiveData, this, _1);

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

  m_face1->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID);
  m_face1->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID);

  BOOST_CHECK(m_face1->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(m_face1->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));

  m_face2->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID);
  m_face2->setLocalControlHeaderFeature(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID);

  BOOST_CHECK(m_face2->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_IN_FACEID));
  BOOST_CHECK(m_face2->isLocalControlHeaderEnabled(LOCAL_CONTROL_HEADER_FEATURE_NEXTHOP_FACEID));

  ////////////////////////////////////////////////////////

  interest1.setIncomingFaceId(11);
  interest1.setNextHopFaceId(111);

  m_face1->sendInterest(interest1);

  data1.setIncomingFaceId(22);
  data1.getLocalControlHeader().setNextHopFaceId(222);

  m_face1->sendData    (data1);

  //

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();

  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);

  // sending allows only IncomingFaceId, receiving allows only NextHopFaceId
  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getLocalControlHeader().hasIncomingFaceId(), false);
  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getLocalControlHeader().hasNextHopFaceId(), false);

  BOOST_CHECK_EQUAL(m_face2_receivedDatas[0].getLocalControlHeader().hasIncomingFaceId(), false);
  BOOST_CHECK_EQUAL(m_face2_receivedDatas[0].getLocalControlHeader().hasNextHopFaceId(), false);

  ////////////////////////////////////////////////////////

  using namespace boost::asio;

  std::vector<const_buffer> interestWithHeader;
  Block iHeader  = interest1.getLocalControlHeader().wireEncode(interest1, true, true);
  Block iPayload = interest1.wireEncode();
  interestWithHeader.push_back(buffer(iHeader.wire(),  iHeader.size()));
  interestWithHeader.push_back(buffer(iPayload.wire(), iPayload.size()));

  std::vector<const_buffer> dataWithHeader;
  Block dHeader  = data1.getLocalControlHeader().wireEncode(data1, true, true);
  Block dPayload = data1.wireEncode();
  dataWithHeader.push_back(buffer(dHeader.wire(),  dHeader.size()));
  dataWithHeader.push_back(buffer(dPayload.wire(), dPayload.size()));

  //

  client->async_send(interestWithHeader, bind(&noOp));
  client->async_send(dataWithHeader, bind(&noOp));

  m_ioRemaining = 2;
  m_ioService.run();
  m_ioService.reset();

  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getLocalControlHeader().hasIncomingFaceId(), false);
  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getLocalControlHeader().hasNextHopFaceId(), true);
  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getNextHopFaceId(), 111);

  BOOST_CHECK_EQUAL(m_face1_receivedDatas[0].getLocalControlHeader().hasIncomingFaceId(), false);
  BOOST_CHECK_EQUAL(m_face1_receivedDatas[0].getLocalControlHeader().hasNextHopFaceId(), false);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
