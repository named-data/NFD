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

#include "face/udp-factory.hpp"
#include "core/network-interface.hpp"

#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceUdp, BaseFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  UdpFactory factory;
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

class FactoryErrorCheck : protected BaseFixture
{
public:
  bool
  isTheSameMulticastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP unicast channel, local "
                  "endpoint is already allocated for a UDP multicast face") == 0;
  }

  bool
  isNotMulticastAddress(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, "
                  "the multicast group given as input is not a multicast address") == 0;
  }

  bool
  isTheSameUnicastEndpoint(const UdpFactory::Error& e) {
    return strcmp(e.what(),
                  "Cannot create the requested UDP multicast face, local "
                  "endpoint is already allocated for a UDP unicast channel") == 0;
  }

  bool
  isLocalEndpointOnDifferentGroup(const UdpFactory::Error& e) {
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
  BOOST_CHECK_EQUAL(channel1->getUri().toString(), "udp4://127.0.0.1:20070");

  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");
  BOOST_CHECK_NE(channel1, channel2);

  shared_ptr<UdpChannel> channel3 = factory.createChannel(interfaceIp, "20070");

  shared_ptr<UdpChannel> channel4 = factory.createChannel("::1", "20071");
  BOOST_CHECK_NE(channel2, channel4);
  BOOST_CHECK_EQUAL(channel4->getUri().toString(), "udp6://[::1]:20071");

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

class FaceCreateFixture : protected BaseFixture
{
public:
  void
  ignore()
  {
  }

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
  UdpFactory factory = UdpFactory();

  factory.createFace(FaceUri("udp4://127.0.0.1"),
                     bind(&FaceCreateFixture::ignore, this),
                     bind(&FaceCreateFixture::failIfError, this, _1));

  factory.createFace(FaceUri("udp4://127.0.0.1/"),
                     bind(&FaceCreateFixture::ignore, this),
                     bind(&FaceCreateFixture::failIfError, this, _1));

  factory.createFace(FaceUri("udp4://127.0.0.1/path"),
                     bind(&FaceCreateFixture::ignore, this),
                     bind(&FaceCreateFixture::checkError, this, _1, "Invalid URI"));

}

class EndToEndFixture : protected BaseFixture
{
public:
  void
  channel1_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    channel1_onFaceCreatedNoCheck(newFace);
  }

  void
  channel1_onFaceCreatedNoCheck(const shared_ptr<Face>& newFace)
  {
    face1 = newFace;
    face1->onReceiveInterest +=
      bind(&EndToEndFixture::face1_onReceiveInterest, this, _1);
    face1->onReceiveData +=
      bind(&EndToEndFixture::face1_onReceiveData, this, _1);
    face1->onFail +=
      bind(&EndToEndFixture::face1_onFail, this);
    BOOST_CHECK_MESSAGE(true, "channel 1 face created");

    faces.push_back(face1);

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

    faces.push_back(face2);

    BOOST_CHECK_MESSAGE(true, "channel 2 face created");
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
  channel3_onFaceCreated(const shared_ptr<Face>& newFace)
  {
    BOOST_CHECK(!static_cast<bool>(face1));
    face3 = newFace;
    faces.push_back(newFace);

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
  channel_onConnectFailedOk(const std::string& reason)
  {
    // it's ok, it was supposed to fail
    limitedIo.afterOp();
  }

  void
  checkFaceList(size_t shouldBe)
  {
    BOOST_CHECK_EQUAL(faces.size(), shouldBe);
  }

  void
  connect(const shared_ptr<UdpChannel>& channel,
          const std::string& remoteHost,
          const std::string& remotePort)
  {
    channel->connect(remoteHost, remotePort,
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
  shared_ptr<Face> face3;

  std::list< shared_ptr<Face> > faces;
};


BOOST_FIXTURE_TEST_CASE(EndToEnd4, EndToEndFixture)
{
  UdpFactory factory;

  factory.createChannel("127.0.0.1", "20071");

  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                     bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));


  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face2));
  BOOST_CHECK_EQUAL(face2->getRemoteUri().toString(), "udp4://127.0.0.1:20070");
  BOOST_CHECK_EQUAL(face2->getLocalUri().toString(), "udp4://127.0.0.1:20071");
  BOOST_CHECK_EQUAL(face2->isLocal(), false);
  BOOST_CHECK_EQUAL(face2->getCounters().getNOutBytes(), 0);
  BOOST_CHECK_EQUAL(face2->getCounters().getNInBytes(), 0);
  // face1 is not created yet

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

  face2->sendInterest(interest2);
  face2->sendData    (data2    );
  face2->sendData    (data2    );
  face2->sendData    (data2    );
  size_t nBytesSent2 = interest2.wireEncode().size() + data2.wireEncode().size() * 3;

  BOOST_CHECK_MESSAGE(limitedIo.run(5,//4 send + 1 listen return
                      time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK_EQUAL(face1->getRemoteUri().toString(), "udp4://127.0.0.1:20071");
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "udp4://127.0.0.1:20070");
  BOOST_CHECK_EQUAL(face1->isLocal(), false);

  face1->sendInterest(interest1);
  face1->sendInterest(interest1);
  face1->sendInterest(interest1);
  face1->sendData    (data1    );

  BOOST_CHECK_MESSAGE(limitedIo.run(4, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 3);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1.getName());



  //checking if the connection accepting mechanism works properly.

  face2->sendData    (data3    );
  face2->sendInterest(interest3);
  nBytesSent2 += data3.wireEncode().size() + interest3.wireEncode().size();

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 2);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 4);

  BOOST_CHECK_EQUAL(face1_receivedInterests[1].getName(), interest3.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [3].getName(), data3.getName());

  const FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.getNInInterests() , 2);
  BOOST_CHECK_EQUAL(counters1.getNInDatas()     , 4);
  BOOST_CHECK_EQUAL(counters1.getNOutInterests(), 3);
  BOOST_CHECK_EQUAL(counters1.getNOutDatas()    , 1);
  BOOST_CHECK_EQUAL(counters1.getNInBytes(), nBytesSent2);

  const FaceCounters& counters2 = face2->getCounters();
  BOOST_CHECK_EQUAL(counters2.getNInInterests() , 3);
  BOOST_CHECK_EQUAL(counters2.getNInDatas()     , 1);
  BOOST_CHECK_EQUAL(counters2.getNOutInterests(), 2);
  BOOST_CHECK_EQUAL(counters2.getNOutDatas()    , 4);
  BOOST_CHECK_EQUAL(counters2.getNOutBytes(), nBytesSent2);
}

BOOST_FIXTURE_TEST_CASE(EndToEnd6, EndToEndFixture)
{
  UdpFactory factory;

  factory.createChannel("::1", "20071");

  factory.createFace(FaceUri("udp://[::1]:20070"),
                     bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                     bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));


  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_REQUIRE(static_cast<bool>(face2));
  BOOST_CHECK_EQUAL(face2->getRemoteUri().toString(), "udp6://[::1]:20070");
  BOOST_CHECK_EQUAL(face2->getLocalUri().toString(), "udp6://[::1]:20071");
  BOOST_CHECK_EQUAL(face2->isLocal(), false);
  // face1 is not created yet

  shared_ptr<UdpChannel> channel1 = factory.createChannel("::1", "20070");
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

  face2->sendInterest(interest2);
  face2->sendData    (data2    );

  BOOST_CHECK_MESSAGE(limitedIo.run(3,//2 send + 1 listen return
                      time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK_EQUAL(face1->getRemoteUri().toString(), "udp6://[::1]:20071");
  BOOST_CHECK_EQUAL(face1->getLocalUri().toString(), "udp6://[::1]:20070");
  BOOST_CHECK_EQUAL(face1->isLocal(), false);

  face1->sendInterest(interest1);
  face1->sendData    (data1    );

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");


  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 1);
  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);

  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2.getName());
  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1.getName());
  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1.getName());



  //checking if the connection accepting mechanism works properly.

  face2->sendData    (data3    );
  face2->sendInterest(interest3);

  BOOST_CHECK_MESSAGE(limitedIo.run(2, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 2);
  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 2);

  BOOST_CHECK_EQUAL(face1_receivedInterests[1].getName(), interest3.getName());
  BOOST_CHECK_EQUAL(face1_receivedDatas    [1].getName(), data3.getName());
}

BOOST_FIXTURE_TEST_CASE(MultipleAccepts, EndToEndFixture)
{
  Interest interest1("ndn:/TpnzGvW9R");
  Interest interest2("ndn:/QWiIMfj5sL");

  UdpFactory factory;

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070");
  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071");

  channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  channel2->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));


  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(faces.size(), 1);

  shared_ptr<UdpChannel> channel3 = factory.createChannel("127.0.0.1", "20072");
  channel3->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel3_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel_onConnectFailed, this, _1));

  shared_ptr<UdpChannel> channel4 = factory.createChannel("127.0.0.1", "20073");

  BOOST_CHECK_NE(channel3, channel4);

  scheduler::schedule(time::milliseconds(500),
                      bind(&EndToEndFixture::connect, this, channel4, "127.0.0.1", "20070"));

  scheduler::schedule(time::milliseconds(400), bind(&EndToEndFixture::checkFaceList, this, 2));

  BOOST_CHECK_MESSAGE(limitedIo.run(2,// 2 connects
                      time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept multiple connections");

  BOOST_CHECK_EQUAL(faces.size(), 3);


  face2->sendInterest(interest1);
  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_CHECK_EQUAL(faces.size(), 4);

  face3->sendInterest(interest2);
  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  //channel1 should have created 2 faces, one when face2 sent an interest, one when face3 sent an interest
  BOOST_CHECK_EQUAL(faces.size(), 5);
  BOOST_CHECK_THROW(channel1->listen(bind(&EndToEndFixture::channel_onFaceCreated,     this, _1),
                                     bind(&EndToEndFixture::channel_onConnectFailedOk, this, _1)),
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
//  limitedIoRemaining = 1;
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
//  BOOST_REQUIRE(static_cast<bool>(face2));
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
//  face2->sendInterest(interest2);
//  face2->sendData    (data2    );
//
//  limitedIoRemaining = 3; //2 send + 1 listen return
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE(static_cast<bool>(face1));
//
//  face1->sendInterest(interest1);
//  face1->sendData    (data1    );
//
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  limitedIoRemaining = 2;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 1);
//  BOOST_REQUIRE_EQUAL(face2_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(face2_receivedDatas    .size(), 1);
//
//  BOOST_CHECK_EQUAL(face1_receivedInterests[0].getName(), interest2.getName());
//  BOOST_CHECK_EQUAL(face1_receivedDatas    [0].getName(), data2.getName());
//  BOOST_CHECK_EQUAL(face2_receivedInterests[0].getName(), interest1.getName());
//  BOOST_CHECK_EQUAL(face2_receivedDatas    [0].getName(), data1.getName());
//
//  //checking if the connection accepting mechanism works properly.
//
//  face2->sendData    (data3    );
//  face2->sendInterest(interest3);
//
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  limitedIoRemaining = 2;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_REQUIRE_EQUAL(face1_receivedInterests.size(), 2);
//  BOOST_REQUIRE_EQUAL(face1_receivedDatas    .size(), 2);
//
//  BOOST_CHECK_EQUAL(face1_receivedInterests[1].getName(), interest3.getName());
//  BOOST_CHECK_EQUAL(face1_receivedDatas    [1].getName(), data3.getName());
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
//  limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_CHECK_EQUAL(faces.size(), 1);
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
//  .scheduleEvent(time::milliseconds(500),
//                 bind(&UdpChannel::connect, channel4,
//                      "::1", "20070",
//                      static_cast<UdpChannel::FaceCreatedCallback>(bind(&EndToEndFixture::
//                                                                        channel_onFaceCreated, this, _1)),
//                      static_cast<UdpChannel::ConnectFailedCallback>(bind(&EndToEndFixture::
//                                                                          channel_onConnectFailed, this, _1))));
//
//  limitedIoRemaining = 2; // 2 connects
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
//  BOOST_CHECK_EQUAL(faces.size(), 3);
//
//
//  face2->sendInterest(interest1);
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  BOOST_CHECK_EQUAL(faces.size(), 4);
//
//  face3->sendInterest(interest2);
//  abortEvent =
//  scheduler.scheduleEvent(time::seconds(1),
//                          bind(&EndToEndFixture::abortTestCase, this,
//                               "UdpChannel error: cannot send or receive Interest/Data packets"));
//  limitedIoRemaining = 1;
//  g_io.run();
//  g_io.reset();
//  scheduler.cancelEvent(abortEvent);
//
//  //channel1 should have created 2 faces, one when face2 sent an interest, one when face3 sent an interest
//  BOOST_CHECK_EQUAL(faces.size(), 5);
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

  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  BOOST_CHECK_EQUAL(channel2->size(), 1);

  BOOST_CHECK(static_cast<bool>(face2));

  // Face::close must be invoked during io run to be counted as an op
  scheduler::schedule(time::milliseconds(100), bind(&Face::close, face2));

  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "FaceClosing error: cannot properly close faces");

  BOOST_CHECK(!static_cast<bool>(face2));

  BOOST_CHECK_EQUAL(channel2->size(), 0);
}

BOOST_FIXTURE_TEST_CASE(ClosingIdleFace, EndToEndFixture)
{
  Interest interest1("ndn:/TpnzGvW9R");
  Interest interest2("ndn:/QWiIMfj5sL");

  UdpFactory factory;

  shared_ptr<UdpChannel> channel1 = factory.createChannel("127.0.0.1", "20070",
                                                          time::seconds(2));
  shared_ptr<UdpChannel> channel2 = factory.createChannel("127.0.0.1", "20071",
                                                          time::seconds(2));

  channel1->listen(bind(&EndToEndFixture::channel1_onFaceCreated,   this, _1),
                   bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  channel2->connect("127.0.0.1", "20070",
                    bind(&EndToEndFixture::channel2_onFaceCreated, this, _1),
                    bind(&EndToEndFixture::channel2_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect or cannot accept connection");

  face2->sendInterest(interest1);
  BOOST_CHECK(!face2->isOnDemand());

  BOOST_CHECK_MESSAGE(limitedIo.run(2,//1 send + 1 listen return
                                      time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");

  BOOST_CHECK_EQUAL(faces.size(), 2);
  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(2)) == LimitedIo::EXCEED_TIME,
                      "Idle face should be still open because has been used recently");
  BOOST_CHECK_EQUAL(faces.size(), 2);
  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_OPS,
                      "Closing idle face error: face should be closed by now");

  //the face on listen should be closed now
  BOOST_CHECK_EQUAL(channel1->size(), 0);
  //checking that face2 has not been closed
  BOOST_CHECK_EQUAL(channel2->size(), 1);
  BOOST_REQUIRE(static_cast<bool>(face2));

  face2->sendInterest(interest2);
  BOOST_CHECK_MESSAGE(limitedIo.run(2,//1 send + 1 listen return
                                      time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot send or receive Interest/Data packets");
  //channel1 should have created a new face by now
  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_CHECK_EQUAL(channel2->size(), 1);
  BOOST_REQUIRE(static_cast<bool>(face1));
  BOOST_CHECK(face1->isOnDemand());

  channel1->connect("127.0.0.1", "20071",
                    bind(&EndToEndFixture::channel1_onFaceCreatedNoCheck, this, _1),
                    bind(&EndToEndFixture::channel1_onConnectFailed, this, _1));

  BOOST_CHECK_MESSAGE(limitedIo.run(1,//1 connect
                                      time::seconds(1)) == LimitedIo::EXCEED_OPS,
                      "UdpChannel error: cannot connect");

  BOOST_CHECK(!face1->isOnDemand());

  //the connect should have set face1 as permanent face,
  //but it shouln't have created any additional faces
  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_CHECK_EQUAL(channel2->size(), 1);
  BOOST_CHECK_MESSAGE(limitedIo.run(1, time::seconds(4)) == LimitedIo::EXCEED_TIME,
                      "Idle face should be still open because it's permanent now");
  //both faces are permanent, nothing should have changed
  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_CHECK_EQUAL(channel2->size(), 1);
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

  UdpFactory factory;
  factory.prohibitEndpoint(udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v6::from_string("2001:db8::1"), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 1);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v6::from_string("2001:db8::1"), 2048),
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v4(), 1024));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 6);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v4::from_string("192.168.2.1"), 1024),
                 udp::Endpoint(address_v4::from_string("192.168.2.2"), 1024),
                 udp::Endpoint(address_v4::from_string("198.51.100.1"), 1024),
                 udp::Endpoint(address_v4::from_string("198.51.100.255"), 1024),
                 udp::Endpoint(address_v4::from_string("255.255.255.255"), 1024),
                 udp::Endpoint(address_v4::from_string("0.0.0.0"), 1024)
               }));

  factory.m_prohibitedEndpoints.clear();
  factory.prohibitEndpoint(udp::Endpoint(address_v6(), 2048));
  BOOST_REQUIRE_EQUAL(factory.m_prohibitedEndpoints.size(), 3);
  BOOST_CHECK((factory.m_prohibitedEndpoints ==
               std::set<udp::Endpoint> {
                 udp::Endpoint(address_v6::from_string("2001:db8::2"), 2048),
                 udp::Endpoint(address_v6::from_string("2001:db8::3"), 2048),
                 udp::Endpoint(address_v6::from_string("::"), 2048),
               }));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
