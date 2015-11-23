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

#include "face/ethernet-factory.hpp"
#include "face/ethernet-transport.hpp"

#include "face/lp-face-wrapper.hpp"
#include "network-interface-fixture.hpp"

#include <pcap/pcap.h>

namespace nfd {
namespace tests {

using nfd::face::tests::NetworkInterfaceFixture;

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestEthernet, NetworkInterfaceFixture)

using nfd::Face;

BOOST_AUTO_TEST_CASE(GetChannels)
{
  EthernetFactory factory;

  auto channels = factory.getChannels();
  BOOST_CHECK_EQUAL(channels.empty(), true);
}

BOOST_AUTO_TEST_CASE(MulticastFacesMap)
{
  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(1);

  EthernetFactory factory;
  auto face1 = factory.createMulticastFace(m_interfaces.front(), ethernet::getBroadcastAddress());
  auto face1bis = factory.createMulticastFace(m_interfaces.front(), ethernet::getBroadcastAddress());
  BOOST_CHECK_EQUAL(face1, face1bis);

  auto face2 = factory.createMulticastFace(m_interfaces.front(), ethernet::getDefaultMulticastAddress());
  BOOST_CHECK_NE(face1, face2);

  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(2);

  auto face3 = factory.createMulticastFace(m_interfaces.back(), ethernet::getBroadcastAddress());
  BOOST_CHECK_NE(face1, face3);
}

BOOST_AUTO_TEST_CASE(UnsupportedFaceCreate)
{
  EthernetFactory factory;

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ether://[08:00:27:01:01:01]"),
                                       ndn::nfd::FACE_PERSISTENCY_PERMANENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ether://[08:00:27:01:01:01]"),
                                       ndn::nfd::FACE_PERSISTENCY_ON_DEMAND,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);

  BOOST_CHECK_THROW(factory.createFace(FaceUri("ether://[08:00:27:01:01:01]"),
                                       ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                       bind([]{}),
                                       bind([]{})),
                    ProtocolFactory::Error);
}

BOOST_AUTO_TEST_CASE(SendPacket)
{
  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(1);

  EthernetFactory factory;
  auto face = factory.createMulticastFace(m_interfaces.front(), ethernet::getDefaultMulticastAddress());

  BOOST_REQUIRE(static_cast<bool>(face));
  BOOST_CHECK_EQUAL(face->isLocal(), false);
  BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  BOOST_CHECK_EQUAL(face->isMultiAccess(), true);
  BOOST_CHECK_EQUAL(face->getRemoteUri().toString(),
                    "ether://[" + ethernet::getDefaultMulticastAddress().toString() + "]");
  BOOST_CHECK_EQUAL(face->getLocalUri().toString(),
                    "dev://" + m_interfaces.front().name);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(face->getCounters().nOutBytes, 0);

  face->onFail.connect([] (const std::string& reason) { BOOST_FAIL(reason); });

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  face->sendInterest(*interest1);
  face->sendData    (*data1    );
  face->sendInterest(*interest2);
  face->sendData    (*data2    );

  BOOST_CHECK_EQUAL(face->getCounters().nOutBytes,
                    interest1->wireEncode().size() +
                    data1->wireEncode().size() +
                    interest2->wireEncode().size() +
                    data2->wireEncode().size());
}

BOOST_AUTO_TEST_CASE(ProcessIncomingPacket)
{
  SKIP_IF_NETWORK_INTERFACE_COUNT_LT(1);

  EthernetFactory factory;
  auto face = factory.createMulticastFace(m_interfaces.front(), ethernet::getDefaultMulticastAddress());
  BOOST_REQUIRE(static_cast<bool>(face));

  auto transport = dynamic_cast<face::EthernetTransport*>(face->getLpFace()->getTransport());
  BOOST_REQUIRE(transport != nullptr);

  std::vector<Interest> recInterests;
  std::vector<Data>     recDatas;

  face->onFail.connect([] (const std::string& reason) { BOOST_FAIL(reason); });
  face->onReceiveInterest.connect(
      [&recInterests] (const Interest& i) { recInterests.push_back(i); });
  face->onReceiveData.connect([&recDatas] (const Data& d) { recDatas.push_back(d); });

  // check that packet data is not accessed if pcap didn't capture anything (caplen == 0)
  static const pcap_pkthdr zeroHeader{};
  transport->processIncomingPacket(&zeroHeader, nullptr);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // runt frame (too short)
  pcap_pkthdr runtHeader{};
  runtHeader.caplen = ethernet::HDR_LEN + 6;
  static const uint8_t packet2[ethernet::HDR_LEN + 6]{};
  transport->processIncomingPacket(&runtHeader, packet2);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, but TLV block has invalid length
  pcap_pkthdr validHeader{};
  validHeader.caplen = ethernet::HDR_LEN + ethernet::MIN_DATA_LEN;
  static const uint8_t packet3[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,       // NDN ethertype
    tlv::Interest,    // TLV type
    0xfd, 0xff, 0xff  // TLV length (invalid because greater than buffer size)
  };
  transport->processIncomingPacket(&validHeader, packet3);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, but TLV block has invalid type
  static const uint8_t packet4[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,       // NDN ethertype
    0x00,             // TLV type (invalid)
    0x00              // TLV length
  };
  transport->processIncomingPacket(&validHeader, packet4);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 2);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame and valid NDNLPv2 packet, but invalid network-layer packet
  static const uint8_t packet5[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,                         // NDN ethertype
    lp::tlv::LpPacket, 0x04,            // start of NDNLPv2 packet
    lp::tlv::Fragment, 0x02,            // single fragment
    0x00,             // TLV type (invalid)
    0x00              // TLV length
  };
  transport->processIncomingPacket(&validHeader, packet5);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 8);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, valid NDNLPv2 packet, and valid NDN (interest) packet
  static const uint8_t packet6[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,                         // NDN ethertype
    lp::tlv::LpPacket, 0x1a,            // start of NDNLPv2 packet
    lp::tlv::Fragment, 0x18,            // single fragment
    tlv::Interest, 0x16,                // start of NDN packet
    0x07, 0x0e, 0x08, 0x07, 0x65, 0x78, // payload
    0x61, 0x6d, 0x70, 0x6c, 0x65, 0x08,
    0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x04,
    0x03, 0xef, 0xe9, 0x7c
  };
  transport->processIncomingPacket(&validHeader, packet6);
  BOOST_CHECK_EQUAL(face->getCounters().nInBytes, 36);
  BOOST_CHECK_EQUAL(recInterests.size(), 1);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestEthernet
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace nfd
