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

#include "face/udp-channel.hpp"
#include "face/udp-face.hpp"
#include "face/udp-factory.hpp"

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"
#include "tests/limited-io.hpp"
#include "face-history.hpp"

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
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "224.0.0.1", "20070"),
                        UdpFactory::Error, isTheSameUnicastEndpoint);

  auto multicastFace1  = factory.createMulticastFace(interfaceIp, "224.0.0.1", "20072");
  auto multicastFace1a = factory.createMulticastFace(interfaceIp, "224.0.0.1", "20072");
  BOOST_CHECK_EQUAL(multicastFace1, multicastFace1a);
  BOOST_CHECK_EQUAL(multicastFace1->isLocal(), false);
  BOOST_CHECK_EQUAL(multicastFace1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(multicastFace1->isMultiAccess(), true);

  //same endpoint of a multicast face
  BOOST_CHECK_EXCEPTION(factory.createChannel(interfaceIp, "20072"),
                        UdpFactory::Error, isTheSameMulticastEndpoint);

  //same multicast endpoint, different group
  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "224.0.0.42", "20072"),
                        UdpFactory::Error, isLocalEndpointOnDifferentGroup);

  BOOST_CHECK_EXCEPTION(factory.createMulticastFace(interfaceIp, "192.168.10.15", "20025"),
                        UdpFactory::Error, isNotMulticastAddress);


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

  factory.createFace(FaceUri("udp4://127.0.0.1:6363"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::checkError, this, _1,
                          "No channels available to connect to 127.0.0.1:6363"));

  factory.createChannel("127.0.0.1", "20071");

  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));
  //test the upgrade
  factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                     ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));

  factory.createFace(FaceUri("udp4://127.0.0.1:20072"),
                     ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                     bind([]{}),
                     bind(&FaceCreateFixture::failIfError, this, _1));
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  UdpFactory factory;

  factory.createChannel("127.0.0.1", "20070");

  BOOST_CHECK_THROW(factory.createFace(FaceUri("udp4://127.0.0.1:20070"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

class EndToEndIpv4
{
public:
  static const std::string
  getScheme()
  {
    return "udp4";
  }

  static const std::string
  getLocalIp()
  {
    return "127.0.0.1";
  }

  static const std::string
  getPort1()
  {
    return "20071";
  }

  static const std::string
  getPort2()
  {
    return "20072";
  }

  static const std::string
  getPort3()
  {
    return "20073";
  }

  static const FaceUri
  getFaceUri1()
  {
    return FaceUri("udp4://127.0.0.1:20071");
  }

  static const FaceUri
  getFaceUri2()
  {
    return FaceUri("udp4://127.0.0.1:20072");
  }

  static const FaceUri
  getFaceUri3()
  {
    return FaceUri("udp4://127.0.0.1:20073");
  }
};

class EndToEndIpv6
{
public:
  static const std::string
  getScheme()
  {
    return "udp6";
  }

  static const std::string
  getLocalIp()
  {
    return "::1";
  }

  static const std::string
  getPort1()
  {
    return "20071";
  }

  static const std::string
  getPort2()
  {
    return "20072";
  }

  static const std::string
  getPort3()
  {
    return "20073";
  }

  static const FaceUri
  getFaceUri1()
  {
    return FaceUri("udp6://[::1]:20071");
  }

  static const FaceUri
  getFaceUri2()
  {
    return FaceUri("udp6://[::1]:20072");
  }

  static const FaceUri
  getFaceUri3()
  {
    return FaceUri("udp6://[::1]:20073");
  }
};

typedef boost::mpl::list<EndToEndIpv4, EndToEndIpv6> EndToEndAddresses;

// end to end communication
BOOST_AUTO_TEST_CASE_TEMPLATE(EndToEnd, A, EndToEndAddresses)
{
  LimitedIo limitedIo;
  UdpFactory factory;

  shared_ptr<UdpChannel> channel1 = factory.createChannel(A::getLocalIp(), A::getPort1());

  // face1 (on channel1) connects to face2 (on channel2, to be created)
  shared_ptr<Face> face1;
  unique_ptr<FaceHistory> history1;
  factory.createFace(A::getFaceUri2(),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     [&] (shared_ptr<Face> newFace) {
                       face1 = newFace;
                       history1.reset(new FaceHistory(*face1, limitedIo));
                       limitedIo.afterOp();
                     },
                     [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::seconds(1));
  BOOST_REQUIRE(face1 != nullptr);
  BOOST_CHECK_EQUAL(face1->getRemoteUri(), A::getFaceUri2());
  BOOST_CHECK_EQUAL(face1->getLocalUri(), A::getFaceUri1());
  BOOST_CHECK_EQUAL(face1->isLocal(), false); // UdpFace is never local
  BOOST_CHECK_EQUAL(face1->getCounters().getNInBytes(), 0);
  BOOST_CHECK_EQUAL(face1->getCounters().getNOutBytes(), 0);

  // channel2 creation must be after face1 creation,
  // otherwise channel2's endpoint would be prohibited
  shared_ptr<UdpChannel> channel2 = factory.createChannel(A::getLocalIp(), A::getPort2());
  shared_ptr<Face> face2;
  unique_ptr<FaceHistory> history2;
  channel2->listen([&] (shared_ptr<Face> newFace) {
                     BOOST_CHECK(face2 == nullptr);
                     face2 = newFace;
                     history2.reset(new FaceHistory(*face2, limitedIo));
                     limitedIo.afterOp();
                   },
                   [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::seconds(1));
  BOOST_CHECK(face2 == nullptr); // face2 shouldn't be created until face1 sends something

  shared_ptr<Interest> interest1 = makeInterest("/I1");
  shared_ptr<Interest> interest2 = makeInterest("/I2");
  shared_ptr<Data> data1 = makeData("/D1");
  shared_ptr<Data> data2 = makeData("/D2");

  // face1 sends to channel2, creates face2
  face1->sendInterest(*interest1);
  face1->sendData(*data1);
  face1->sendData(*data1);
  face1->sendData(*data1);
  size_t nBytesSent1 = interest1->wireEncode().size() + 3 * data1->wireEncode().size();

  limitedIo.run(5, time::seconds(1)); // 1 accept, 4 receives

  BOOST_REQUIRE(face2 != nullptr);
  BOOST_CHECK_EQUAL(face2->getRemoteUri(), A::getFaceUri1());
  BOOST_CHECK_EQUAL(face2->getLocalUri(), A::getFaceUri2());
  BOOST_CHECK_EQUAL(face2->isLocal(), false); // UdpFace is never local
  BOOST_CHECK_EQUAL(face2->getCounters().getNInBytes(), nBytesSent1);
  BOOST_CHECK_EQUAL(face2->getCounters().getNOutBytes(), 0);

  BOOST_REQUIRE_EQUAL(history2->receivedInterests.size(), 1);
  BOOST_CHECK_EQUAL(history2->receivedInterests.front().getName(), interest1->getName());
  BOOST_REQUIRE_EQUAL(history2->receivedData.size(), 3);
  BOOST_CHECK_EQUAL(history2->receivedData.front().getName(), data1->getName());

  // face2 sends to face1
  face2->sendInterest(*interest2);
  face2->sendInterest(*interest2);
  face2->sendInterest(*interest2);
  face2->sendData(*data2);
  size_t nBytesSent2 = 3 * interest2->wireEncode().size() + data2->wireEncode().size();

  limitedIo.run(4, time::seconds(1)); // 4 receives

  BOOST_REQUIRE_EQUAL(history1->receivedInterests.size(), 3);
  BOOST_CHECK_EQUAL(history1->receivedInterests.front().getName(), interest2->getName());
  BOOST_REQUIRE_EQUAL(history1->receivedData.size(), 1);
  BOOST_CHECK_EQUAL(history1->receivedData.front().getName(), data2->getName());

  // counters
  const FaceCounters& counters1 = face1->getCounters();
  BOOST_CHECK_EQUAL(counters1.getNInInterests(), 3);
  BOOST_CHECK_EQUAL(counters1.getNInDatas(), 1);
  BOOST_CHECK_EQUAL(counters1.getNOutInterests(), 1);
  BOOST_CHECK_EQUAL(counters1.getNOutDatas(), 3);
  BOOST_CHECK_EQUAL(counters1.getNInBytes(), nBytesSent2);
  BOOST_CHECK_EQUAL(counters1.getNOutBytes(), nBytesSent1);

  const FaceCounters& counters2 = face2->getCounters();
  BOOST_CHECK_EQUAL(counters2.getNInInterests(), 1);
  BOOST_CHECK_EQUAL(counters2.getNInDatas(), 3);
  BOOST_CHECK_EQUAL(counters2.getNOutInterests(), 3);
  BOOST_CHECK_EQUAL(counters2.getNOutDatas(), 1);
  BOOST_CHECK_EQUAL(counters2.getNInBytes(), nBytesSent1);
  BOOST_CHECK_EQUAL(counters2.getNOutBytes(), nBytesSent2);
}

// channel accepting multiple incoming connections
BOOST_AUTO_TEST_CASE_TEMPLATE(MultipleAccepts, A, EndToEndAddresses)
{
  LimitedIo limitedIo;
  UdpFactory factory;

  // channel1 is listening
  shared_ptr<UdpChannel> channel1 = factory.createChannel(A::getLocalIp(), A::getPort1());
  std::vector<shared_ptr<Face>> faces1;
  channel1->listen([&] (shared_ptr<Face> newFace) {
                     faces1.push_back(newFace);
                     limitedIo.afterOp();
                   },
                   [] (const std::string& reason) { BOOST_ERROR(reason); });

  // face2 (on channel2) connects to channel1
  shared_ptr<UdpChannel> channel2 = factory.createChannel(A::getLocalIp(), A::getPort2());
  BOOST_CHECK_NE(channel1, channel2);
  shared_ptr<Face> face2;
  boost::asio::ip::address ipAddress2 = boost::asio::ip::address::from_string(A::getLocalIp());
  udp::Endpoint endpoint2(ipAddress2, boost::lexical_cast<uint16_t>(A::getPort1()));
  channel2->connect(endpoint2,
                    ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                    [&] (shared_ptr<Face> newFace) {
                      face2 = newFace;
                      limitedIo.afterOp();
                    },
                    [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::seconds(1)); // 1 create (on channel2)
  BOOST_REQUIRE(face2 != nullptr);
  BOOST_CHECK_EQUAL(faces1.size(), 0); // channel1 won't create face until face2 sends something

  // face3 (on channel3) connects to channel1
  shared_ptr<UdpChannel> channel3 = factory.createChannel(A::getLocalIp(), A::getPort3());
  BOOST_CHECK_NE(channel1, channel3);
  shared_ptr<Face> face3;
  boost::asio::ip::address ipAddress3 = boost::asio::ip::address::from_string(A::getLocalIp());
  udp::Endpoint endpoint3(ipAddress3, boost::lexical_cast<uint16_t>(A::getPort1()));
  channel3->connect(endpoint3,
                    ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                    [&] (shared_ptr<Face> newFace) {
                      face3 = newFace;
                      limitedIo.afterOp();
                    },
                    [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::seconds(1)); // 1 create (on channel3)
  BOOST_REQUIRE(face3 != nullptr);
  BOOST_CHECK_EQUAL(faces1.size(), 0); // channel1 won't create face until face3 sends something

  // face2 sends to channel1
  shared_ptr<Interest> interest2 = makeInterest("/I2");
  face2->sendInterest(*interest2);
  limitedIo.run(1, time::milliseconds(100)); // 1 accept (on channel1)
  BOOST_REQUIRE_EQUAL(faces1.size(), 1);
  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_CHECK_EQUAL(faces1.at(0)->getRemoteUri(), A::getFaceUri2());

  // face3 sends to channel1
  shared_ptr<Data> data3 = makeData("/D3");
  face3->sendData(*data3);
  limitedIo.run(1, time::milliseconds(100)); // 1 accept (on channel1)
  BOOST_REQUIRE_EQUAL(faces1.size(), 2);
  BOOST_CHECK_EQUAL(channel1->size(), 2);
  BOOST_CHECK_EQUAL(faces1.at(1)->getRemoteUri(), A::getFaceUri3());
}

// manually close a face
BOOST_AUTO_TEST_CASE_TEMPLATE(ManualClose, A, EndToEndAddresses)
{
  LimitedIo limitedIo;
  UdpFactory factory;

  shared_ptr<UdpChannel> channel1 = factory.createChannel(A::getLocalIp(), A::getPort1());
  shared_ptr<Face> face1;
  unique_ptr<FaceHistory> history1;
  factory.createFace(A::getFaceUri2(),
                     ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                     [&] (shared_ptr<Face> newFace) {
                       face1 = newFace;
                       history1.reset(new FaceHistory(*face1, limitedIo));
                       limitedIo.afterOp();
                     },
                     [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::milliseconds(100));
  BOOST_REQUIRE(face1 != nullptr);
  BOOST_CHECK_EQUAL(channel1->size(), 1);

  face1->close();
  getGlobalIoService().poll();
  BOOST_CHECK_EQUAL(history1->failures.size(), 1);
  BOOST_CHECK_EQUAL(channel1->size(), 0);
}

// automatically close an idle incoming face
BOOST_AUTO_TEST_CASE_TEMPLATE(IdleClose, A, EndToEndAddresses)
{
  LimitedIo limitedIo;
  UdpFactory factory;

  // channel1 is listening
  shared_ptr<UdpChannel> channel1 = factory.createChannel(A::getLocalIp(), A::getPort1(),
                                                          time::seconds(2));
  shared_ptr<Face> face1;
  unique_ptr<FaceHistory> history1;
  channel1->listen([&] (shared_ptr<Face> newFace) {
                     BOOST_CHECK(face1 == nullptr);
                     face1 = newFace;
                     history1.reset(new FaceHistory(*face1, limitedIo));
                     limitedIo.afterOp();
                   },
                   [] (const std::string& reason) { BOOST_ERROR(reason); });

  // face2 (on channel2) connects to channel1
  shared_ptr<UdpChannel> channel2 = factory.createChannel(A::getLocalIp(), A::getPort2(),
                                                          time::seconds(2));
  shared_ptr<Face> face2;
  unique_ptr<FaceHistory> history2;
  boost::asio::ip::address ipAddress = boost::asio::ip::address::from_string(A::getLocalIp());
  udp::Endpoint endpoint(ipAddress, boost::lexical_cast<uint16_t>(A::getPort1()));
  channel2->connect(endpoint,
                    ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                    [&] (shared_ptr<Face> newFace) {
                      face2 = newFace;
                      history2.reset(new FaceHistory(*face2, limitedIo));
                      limitedIo.afterOp();
                    },
                    [] (const std::string& reason) { BOOST_ERROR(reason); });

  limitedIo.run(1, time::milliseconds(100)); // 1 create (on channel2)
  BOOST_REQUIRE(face2 != nullptr);
  BOOST_CHECK_EQUAL(face2->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(face2->isMultiAccess(), false);

  // face2 sends to channel1, creates face1
  shared_ptr<Interest> interest2 = makeInterest("/I2");
  face2->sendInterest(*interest2);

  limitedIo.run(2, time::seconds(1)); // 1 accept (on channel1), 1 receive (on face1)
  BOOST_CHECK_EQUAL(channel1->size(), 1);
  BOOST_REQUIRE(face1 != nullptr);
  BOOST_CHECK_EQUAL(face1->getPersistency(), ndn::nfd::FACE_PERSISTENCY_ON_DEMAND);
  BOOST_CHECK_EQUAL(face1->isMultiAccess(), false);

  limitedIo.defer(time::seconds(1));
  BOOST_CHECK_EQUAL(history1->failures.size(), 0); // face1 is not idle
  BOOST_CHECK_EQUAL(history2->failures.size(), 0); // face2 is outgoing face and never closed

  limitedIo.defer(time::seconds(4));
  BOOST_CHECK_EQUAL(history1->failures.size(), 1); // face1 is idle and automatically closed
  BOOST_CHECK_EQUAL(channel1->size(), 0);
  BOOST_CHECK_EQUAL(history2->failures.size(), 0); // face2 is outgoing face and never closed
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
