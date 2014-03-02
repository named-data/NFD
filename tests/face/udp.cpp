/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/udp-factory.hpp"
#include "core/face-uri.hpp"
#include <ndn-cpp-dev/security/key-chain.hpp>

#include "tests/test-common.hpp"
#include "tests/core/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceUdp, BaseFixture)

class FactoryErrorCheck : protected BaseFixture
{
public:
  bool isTheSameMulticastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP unicast channel, local "
                  "endpoint is already allocated for a UDP multicast face") == 0;
  }

  bool isNotMulticastAddress(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, "
                  "the multicast group given as input is not a multicast address") == 0;
  }

  bool isTheSameUnicastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP unicast channel") == 0;
  }

  bool isLocalEndpointOnDifferentGroup(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP multicast face "
                  "on a different multicast group") == 0;
  }
};

BOOST_FIXTURE_TEST_CASE(ChannelMapUdp, FactoryErrorCheck)
{
  using boost::asio::ip::udp;

  UdpFactory factory = UdpFactory();

  //to instantiate multicast face on a specific ip address, change interfaceIp
  std::string interfaceIp = "0.0.0.0";

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<UdpChannel> channel1a = factory.createChannel("127.0.0.1", "20070");
  BOOST_CHECK_EQUAL(channel1, channel1a);

  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  shared_ptr<UdpChannel> channel3 = factory.createChannel(interfaceIp, "20070");

  //same endpoint of a unicast channel
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp,
                                                    "224.0.0.1",
                                                    "20070"),
                        UdpFactory::Error,
                        isTheSameUnicastEndpoint);


  shared_ptr<MulticastUdpFace> multicastFace1 = factory.createMulticastFace(interfaceIp,
                                                                            "224.0.0.1",
                                                                            "20072");
  shared_ptr<MulticastUdpFace> multicastFace1a = factory.createMulticastFace(interfaceIp,
                                                                            "224.0.0.1",
                                                                            "20072");
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);


  //same endpoint of a multicast face
  BOOST_CHECK_EXCEPTION(factory.createChannel(interfaceIp, "20072"),
                        UdpFactory::Error,
                        isTheSameMulticastEndpoint);

  //same multicast endpoint, different group
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp,
                                                    "224.0.0.42",
                                                    "20072"),
                        UdpFactory::Error,
                        isLocalEndpointOnDifferentGroup);

  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp,
                                                    "192.168.10.15",
                                                    "20025"),
                        UdpFactory::Error,
                        isNotMulticastAddress);



//  //Test commented because it required to be run in a machine that can resolve ipv6 query
//  shared_ptr<UdpChannel> channel1v6 = factory.createChannel(//"::1",
//                                                     "fe80::5e96:9dff:fe7d:9c8d%en1",
//                                                     //"fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                     "20070");
//
//  //the creation of multicastFace2 works properly. It has been disable because it needs an IP address of
//  //an available network interface (different from the first one used)
//  shared_ptr<MulticastUdpFace> multicastFace2 = factory.createMulticastFace("192.168.1.17",
//                                                                            "224.0.0.1",
//                                                                            "20073");
//  BOOST_CHECK_NE(multicastFace1, multicastFace2);
//
//
//  //ipv6 - work in progress
//  shared_ptr<MulticastUdpFace> multicastFace3 = factory.createMulticastFace("fe80::5e96:9dff:fe7d:9c8d%en1",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  shared_ptr<MulticastUdpFace> multicastFace4 = factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  BOOST_CHECK_EQUAL(multicastFace3, multicastFace4);
//
//  shared_ptr<MulticastUdpFace> multicastFace5 = factory.createMulticastFace("::1",
//                                                                            "FF01:0:0:0:0:0:0:2",
//                                                                            "20073");
//
//  BOOST_CHECK_NE(multicastFace3, multicastFace5);
//
//  //same local ipv6 endpoint for a different multicast group
//  BOOST_CHECK_THROW(factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                "FE01:0:0:0:0:0:0:2",
//                                                "20073"),
//                    UdpFactory::Error);
//
//  //same local ipv6 (expect for th port number) endpoint for a different multicast group
//  BOOST_CHECK_THROW(factory.createMulticastFace("fe80::aa54:b2ff:fe08:27b8%wlan0",
//                                                "FE01:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);
//
//  BOOST_CHECK_THROW(factory.createMulticastFace("fa80::20a:9dff:fef6:12ff",
//                                                "FE12:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);
//
//  //not a multicast ipv6
//  BOOST_CHECK_THROW(factory.createMulticastFace("fa80::20a:9dff:fef6:12ff",
//                                                "A112:0:0:0:0:0:0:2",
//                                                "20075"),
//                    UdpFactory::Error);



}

class EndToEndFixture : protected BaseFixture
{
public:
  void
  channel1_onFaceCreated(const shared_ptr<UdpFace>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(m_face1));
    m_face1 = newFace;
    m_face1->onReceiveInterest +=
      bind(&EndToEndFixture::face1_onReceiveInterest, this, _1);
    m_face1->onReceiveData +=
      bind(&EndToEndFixture::face1_onReceiveData, this, _1);
    m_face1->onFail +=
      bind(&EndToEndFixture::face1_onFail, this);
    BOOST_CHECK_MESSAGE(true, "channel 1 face created");

    m_faces.push_back(m_face1);

    m_limitedIo.afterOp();
  }

  void
  channel1_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    m_limitedIo.afterOp();
  }

  void
  face1_onReceiveInterest(const Interest& interest)
  {
    m_face1_receivedInterests.push_back(interest);

    m_limitedIo.afterOp();
  }

  void
  face1_onReceiveData(const Data& data)
  {
    m_face1_receivedDatas.push_back(data);

    m_limitedIo.afterOp();
  }

  void
  face1_onFail()
  {
    m_face1.reset();
    m_limitedIo.afterOp();
  }

  void
  channel2_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(m_face2));
    m_face2 = newFace;
    m_face2->onReceiveInterest +=
      bind(&EndToEndFixture::face2_onReceiveInterest, this, _1);
    m_face2->onReceiveData +=
      bind(&EndToEndFixture::face2_onReceiveData, this, _1);
    m_face2->onFail +=
      bind(&EndToEndFixture::face2_onFail, this);

    m_faces.push_back(m_face2);

    BOOST_CHECK_MESSAGE(true, "channel 2 face created");
    m_limitedIo.afterOp();
  }

  void
  channel2_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    m_limitedIo.afterOp();
  }

  void
  face2_onReceiveInterest(const Interest& interest)
  {
    m_face2_receivedInterests.push_back(interest);

    m_limitedIo.afterOp();
  }

  void
  face2_onReceiveData(const Data& data)
  {
    m_face2_receivedDatas.push_back(data);

    m_limitedIo.afterOp();
  }

  void
  face2_onFail()
  {
    m_face2.reset();
    m_limitedIo.afterOp();
  }

  void
  channel3_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(m_face1));
    m_face3 = newFace;
    m_faces.push_back(newFace);

    m_limitedIo.afterOp();
  }


  void
  channel_onFaceCreated(const shared_ptr<UdpFace>& newFace)
  {
    m_faces.push_back(newFace);
    m_limitedIo.afterOp();
  }

  void
  channel_onConnectFailed(const std::string& reason)
  {
    BOOST_CHECK_MESSAGE(false, reason);

    m_limitedIo.afterOp();
  }

  void
  channel_onConnectFailedOk(const std::string& reason)
  {
    //it's ok, it was supposed to fail
    m_limitedIo.afterOp();
  }

  void
  checkFaceList(size_t shouldBe)
  {
    BOOST_CHECK_EQUAL(m_faces.size(), shouldBe);
  }

public:
  LimitedIo m_limitedIo;

  shared_ptr<Face> m_face1;
  std::vector<Interest> m_face1_receivedInterests;
  std::vector<Data> m_face1_receivedDatas;
  shared_ptr<Face> m_face2;
  std::vector<Interest> m_face2_receivedInterests;
  std::vector<Data> m_face2_receivedDatas;

  shared_ptr<Face> m_face3;


  std::list< shared_ptr<Face> > m_faces;
};


BOOST_FIXTURE_TEST_CASE(EndToEnd, EndToEndFixture)
{
  UdpFactory factory;

  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");


  //channel2->connect("127.0.0.1", "20070",
  //                  bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
  //                  bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                     bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));


  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(m_face2));

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));


  Interest interest1("ndn:/TpnzGvW9R");
  Data     data1    ("ndn:/KfczhUqVix");
  data1.setContent(0, 0);
  Interest interest2("ndn:/QWiIMfj5sL");
  Data     data2    ("ndn:/XNBV796f");
  data2.setContent(0, 0);
  Interest interest3("ndn:/QWiIhjgkj5sL");
  Data     data3    ("ndn:/XNBV794f");
  data3.setContent(0, 0);


  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                        reinterpret_cast<const uint8_t*>(0),
                                        0));

  // set fake signature on data1 and data2
  data1.setSignature(fakeSignature);
  data2.setSignature(fakeSignature);
  data3.setSignature(fakeSignature);

  m_face2->sendInterest(interest2);
  m_face2->sendData    (data2    );

  BOOST_CHECK_MESSAGE(m_limitedIo.run(3,//2 send + 1 listen return
                      time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE(static_cast<bool>(m_face1));

  m_face1->sendInterest(interest1);
  m_face1->sendData    (data1    );

  BOOST_CHECK_MESSAGE(m_limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");


  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(m_face2_receivedDatas    [0].getName(), data1.getName());



  //checking if the connection accepting mechanism works properly.

  m_face2->sendData    (data3    );
  m_face2->sendInterest(interest3);

  BOOST_CHECK_MESSAGE(m_limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 2);
  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 2);

  BOOST_CHECK_EQUAL(m_face1_receivedInterests[1].getName(), interest3.getName());
  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [1].getName(), data3.getName());

}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  Interest interest1("ndn:/TpnzGvW9R");
  Interest interest2("ndn:/QWiIMfj5sL");
  Interest interest3("ndn:/QWiIhjgkj5sL");

  UdpFactory factory;

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");



  channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  channel2->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));


  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(m_faces.size(), 1);

  shared_ptr<UdpChannel> channel3 = factory.createChannel("127.0.0.1", "20072");
  channel3->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel3_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  shared_ptr<UdpChannel> channel4 = factory.createChannel("127.0.0.1", "20073");

  BOOST_CHECK_NE(channel3, channel4);

  scheduler::schedule(time::seconds(0.5),
           bind(&UdpChannel::connect, channel4, "127.0.0.1", "20070",
                // does not work without static_cast
                static_cast<UdpChannel::FaceCreatedCallback>(
                    bind(&EndToEndFixture::channel_onFaceCreated, this, _1)),
                static_cast<UdpChannel::ConnectFailedCallback>(
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1))));

  scheduler::schedule(time::seconds(0.4), bind(&EndToEndFixture::checkFaceList, this, 2));

  BOOST_CHECK_MESSAGE(m_limitedIo.run(2,// 2 connects
                      time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept multiple connections");

  BOOST_CHECK_EQUAL(m_faces.size(), 3);


  m_face2->sendInterest(interest1);
  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_CHECK_EQUAL(m_faces.size(), 4);

  m_face3->sendInterest(interest2);
  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  //channel1 should have created 2 faces, one when m_face2 sent an interest, one when m_face3 sent an interest
  BOOST_CHECK_EQUAL(m_faces.size(), 5);
  BOOST_CHECK_THROW(channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,
                                          this,
                                          _1),
                                    bind(&EndToEndFixture::channel_onConnectFailedOk,
                                         this,
                                         _1)),
                    UdpChannel::Error);
}

//Test commented because it required to be run in a machine that can resolve ipv6 query
//BOOST_FIXTURE_TEST_CASE(EndToEndIpv6, EndToEndFixture)
//{
//  UdpFactory factory = UdpFactory();
//  Scheduler scheduler(g_io); // to limit the amount of time the test may take
//
//  EventId abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                              "UdpChannel error: cannot connect or cannot accept connection"));
//
//  m_limitedIoRemaining = 1;
//
//  shared_ptr<UdpChannel> channel1 = factory.createChannel("::1", "20070");
//  shared_ptr<UdpChannel> channel2 = factory.createChannel("::1", "20071");
//
//  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
//                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));
//
//  channel2->connect("::1", "20070",
//                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
//                    bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE(static_cast<bool>(m_face2));
//
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//
//  Interest interest1("ndn:/TpnzGvW9R");
//  Data     data1    ("ndn:/KfczhUqVix");
//  data1.setContent(0, 0);
//  Interest interest2("ndn:/QWiIMfj5sL");
//  Data     data2    ("ndn:/XNBV796f");
//  data2.setContent(0, 0);
//  Interest interest3("ndn:/QWiIhjgkj5sL");
//  Data     data3    ("ndn:/XNBV794f");
//  data3.setContent(0, 0);
//
//  ndn::SignatureSha256WithRsa fakeSignature;
//  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
//
//  // set fake signature on data1 and data2
//  data1.setSignature(fakeSignature);
//  data2.setSignature(fakeSignature);
//  data3.setSignature(fakeSignature);
//
//  m_face2->sendInterest(interest2);
//  m_face2->sendData    (data2    );
//
//  m_limitedIoRemaining = 3; //2 send + 1 listen return
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE(static_cast<bool>(m_face1));
//
//  m_face1->sendInterest(interest1);
//  m_face1->sendData    (data1    );
//
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  m_limitedIoRemaining = 2;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);
//
//  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest2.getName());
//  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data2.getName());
//  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getName(), interest1.getName());
//  BOOST_CHECK_EQUAL(m_face2_receivedDatas    [0].getName(), data1.getName());
//
//  //checking if the connection accepting mechanism works properly.
//
//  m_face2->sendData    (data3    );
//  m_face2->sendInterest(interest3);
//
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  m_limitedIoRemaining = 2;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 2);
//  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 2);
//
//  BOOST_CHECK_EQUAL(m_face1_receivedInterests[1].getName(), interest3.getName());
//  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [1].getName(), data3.getName());
//
//}


//Test commented because it required to be run in a machine that can resolve ipv6 query
//BOOST_FIXTURE_TEST_CASE(MultipleAcceptsIpv6, EndToEndFixture)
//{
//  Interest interest1("ndn:/TpnzGvW9R");
//  Interest interest2("ndn:/QWiIMfj5sL");
//  Interest interest3("ndn:/QWiIhjgkj5sL");
//
//  UdpFactory factory = UdpFactory();
//  Scheduler scheduler(g_io); // to limit the amount of time the test may take
//
//  EventId abortEvent =
//  scheduler.scheduleEvent(time::seconds(4),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot connect or cannot accept connection"));
//
//  shared_ptr<UdpChannel> channel1 = factory.createChannel("::1", "20070");
//  shared_ptr<UdpChannel> channel2 = factory.createChannel("::1", "20071");
//
//  channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
//                   bind(&EndToEndFixture::channel_onConnectFailed, this, _1));
//
//  channel2->connect("::1", "20070",
//                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
//                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));
//
//  m_limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_CHECK_EQUAL(m_faces.size(), 1);
//
//  shared_ptr<UdpChannel> channel3 = factory.createChannel("::1", "20072");
//  channel3->connect("::1", "20070",
//                    bind(&EndToEndFixture::channel3_onFaceCreated, this, _1),
//                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));
//
//  shared_ptr<UdpChannel> channel4 = factory.createChannel("::1", "20073");
//
//  BOOST_CHECK_NE(channel3, channel4);
//
//  scheduler
//  .scheduleEvent(time::seconds(0.5),
//                 bind(&UdpChannel::connect, channel4,
//                      "::1", "20070",
//                      static_cast<UdpChannel::FaceCreatedCallback>(bind(&EndToEndFixture::
//                                                                        channel_onFaceCreated, this, _1)),
//                      static_cast<UdpChannel::ConnectFailedCallback>(bind(&EndToEndFixture::
//                                                                          channel_onConnectFailed, this, _1))));
//
//  m_limitedIoRemaining = 2; // 2 connects
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(4),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot connect or cannot accept multiple connections"));
//
//  scheduler.scheduleEvent(time::seconds(0.4),
//                          bind(&EndToEndFixture::checkFaceList, this, 2));
//
//  g_io.run();
//  g_io.reset();
//
//  BOOST_CHECK_EQUAL(m_faces.size(), 3);
//
//
//  m_face2->sendInterest(interest1);
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  m_limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_CHECK_EQUAL(m_faces.size(), 4);
//
//  m_face3->sendInterest(interest2);
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  m_limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  //channel1 should have created 2 faces, one when m_face2 sent an interest, one when m_face3 sent an interest
//  BOOST_CHECK_EQUAL(m_faces.size(), 5);
//  BOOST_CHECK_THROW(channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,
//                                          this,
//                                          _1),
//                                      bind(&EndToEndFixture::channel_onConnectFailedOk,
//                                          this,
//                                          _1)),
//                    UdpChannel::Error);
//}


BOOST_FIXTURE_TEST_CASE(FaceClosing, EndToEndFixture)
{
  UdpFactory factory = UdpFactory();

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  channel2->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(channel2->size(), 1);

  BOOST_CHECK(static_cast<bool>(m_face2));

  // Face::close must be invoked during io run to be counted as an op
  scheduler::schedule(time::seconds(0.1), bind(&Face::close, m_face2));

  BOOST_CHECK_MESSAGE(m_limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "FaceClosing error: cannot properly close faces");

  BOOST_CHECK(!static_cast<bool>(m_face2));

  BOOST_CHECK_EQUAL(channel2->size(), 0);
}

//BOOST_FIXTURE_TEST_CASE(MulticastFace, EndToEndFixture)
//{
//  //to instantiate multicast face on a specific ip address, change interfaceIp
//  std::string interfaceIp = "0.0.0.0";
//
//  UdpFactory factory = UdpFactory();
//  Scheduler scheduler(g_io); // to limit the amount of time the test may take
//
//  shared_ptr<MulticastUdpFace> multicastFace1 =
//    factory.createMulticastFace(interfaceIp, "224.0.0.1", "20072");
//
//  BOOST_REQUIRE(static_cast<bool>(multicastFace1));
//
//  channel1_onFaceCreated(multicastFace1);
//
//
//  EventId abortEvent =
//  scheduler.scheduleEvent(time::seconds(10),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "MulticastFace error: cannot send or receive Interest/Data packets"));
//
//  Interest interest1("ndn:/TpnzGvW9R");
//  Data     data1    ("ndn:/KfczhUqVix");
//  data1.setContent(0, 0);
//  Interest interest2("ndn:/QWiIMfj5sL");
//  Data     data2    ("ndn:/XNBV796f");
//  data2.setContent(0, 0);
//  Interest interest3("ndn:/QWiIhjgkj5sL");
//  Data     data3    ("ndn:/XNBV794f");
//  data3.setContent(0, 0);
//
//
//  ndn::SignatureSha256WithRsa fakeSignature;
//  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
//
//  // set fake signature on data1 and data2
//  data1.setSignature(fakeSignature);
//  data2.setSignature(fakeSignature);
//  data3.setSignature(fakeSignature);
//
//  multicastFace1->sendInterest(interest1);
//  multicastFace1->sendData    (data1    );
//
//  g_io.reset();
//  m_limitedIoRemaining = 2;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
//
//  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest1.getName());
//  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data1.getName());
//}



BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
