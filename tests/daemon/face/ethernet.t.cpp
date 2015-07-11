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

#include "face/ethernet-face.hpp"
#include "face/ethernet-factory.hpp"

#include "core/network-interface.hpp"
#include "tests/test-common.hpp"

#include <pcap/pcap.h>

namespace nfd {
namespace tests {

class InterfacesFixture : protected BaseFixture
{
protected:
  InterfacesFixture()
  {
    EthernetFactory factory;

    for (const auto& netif : listNetworkInterfaces()) {
      if (!netif.isLoopback() && netif.isUp()) {
        try {
          factory.createMulticastFace(netif, ethernet::getBroadcastAddress());
        }
        catch (Face::Error&) {
          continue;
        }

        m_interfaces.push_back(netif);
      }
    }
  }

protected:
  std::vector<NetworkInterfaceInfo> m_interfaces;
};

BOOST_FIXTURE_TEST_SUITE(FaceEthernet, InterfacesFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  EthernetFactory factory;

  auto channels = factory.getChannels();
  BOOST_CHECK_EQUAL(channels.empty(), true);
}

BOOST_AUTO_TEST_CASE(MulticastFacesMap)
{
  if (m_interfaces.empty()) {
    BOOST_WARN_MESSAGE(false, "No interfaces available for pcap, "
                              "cannot perform MulticastFacesMap test");
    return;
  }

  EthernetFactory factory;
  shared_ptr<EthernetFace> face1 = factory.createMulticastFace(m_interfaces.front(),
                                                               ethernet::getBroadcastAddress());
  shared_ptr<EthernetFace> face1bis = factory.createMulticastFace(m_interfaces.front(),
                                                                  ethernet::getBroadcastAddress());
  BOOST_CHECK_EQUAL(face1, face1bis);

  if (m_interfaces.size() > 1) {
    shared_ptr<EthernetFace> face2 = factory.createMulticastFace(m_interfaces.back(),
                                                                 ethernet::getBroadcastAddress());
    BOOST_CHECK_NE(face1, face2);
  }
  else {
    BOOST_WARN_MESSAGE(false, "Only one interface available for pcap, "
                              "cannot test second EthernetFace creation");
  }

  shared_ptr<EthernetFace> face3 = factory.createMulticastFace(m_interfaces.front(),
                                     ethernet::getDefaultMulticastAddress());
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
  if (m_interfaces.empty()) {
    BOOST_WARN_MESSAGE(false, "No interfaces available for pcap, "
                              "cannot perform SendPacket test");
    return;
  }

  EthernetFactory factory;
  shared_ptr<EthernetFace> face = factory.createMulticastFace(m_interfaces.front(),
                                    ethernet::getDefaultMulticastAddress());

  BOOST_REQUIRE(static_cast<bool>(face));
  BOOST_CHECK_EQUAL(face->isLocal(), false);
  BOOST_CHECK_EQUAL(face->getPersistency(), ndn::nfd::FACE_PERSISTENCY_PERSISTENT);
  BOOST_CHECK_EQUAL(face->isMultiAccess(), true);
  BOOST_CHECK_EQUAL(face->getRemoteUri().toString(),
                    "ether://[" + ethernet::getDefaultMulticastAddress().toString() + "]");
  BOOST_CHECK_EQUAL(face->getLocalUri().toString(),
                    "dev://" + m_interfaces.front().name);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 0);
  BOOST_CHECK_EQUAL(face->getCounters().getNOutBytes(), 0);

  face->onFail.connect([] (const std::string& reason) { BOOST_FAIL(reason); });

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  face->sendInterest(*interest1);
  face->sendData    (*data1    );
  face->sendInterest(*interest2);
  face->sendData    (*data2    );

  BOOST_CHECK_EQUAL(face->getCounters().getNOutBytes(),
                    14 * 4 + // 4 NDNLP headers
                    interest1->wireEncode().size() +
                    data1->wireEncode().size() +
                    interest2->wireEncode().size() +
                    data2->wireEncode().size());

//  m_ioRemaining = 4;
//  m_ioService.run();
//  m_ioService.reset();

//  BOOST_REQUIRE_EQUAL(m_face1_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face1_receivedDatas    .size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face2_receivedInterests.size(), 1);
//  BOOST_REQUIRE_EQUAL(m_face2_receivedDatas    .size(), 1);

//  BOOST_CHECK_EQUAL(m_face1_receivedInterests[0].getName(), interest2.getName());
//  BOOST_CHECK_EQUAL(m_face1_receivedDatas    [0].getName(), data2.getName());
//  BOOST_CHECK_EQUAL(m_face2_receivedInterests[0].getName(), interest1.getName());
//  BOOST_CHECK_EQUAL(m_face2_receivedDatas    [0].getName(), data1.getName());
}

BOOST_AUTO_TEST_CASE(ProcessIncomingPacket)
{
  if (m_interfaces.empty()) {
    BOOST_WARN_MESSAGE(false, "No interfaces available for pcap, "
                              "cannot perform ProcessIncomingPacket test");
    return;
  }

  EthernetFactory factory;
  shared_ptr<EthernetFace> face = factory.createMulticastFace(m_interfaces.front(),
                                    ethernet::getDefaultMulticastAddress());
  BOOST_REQUIRE(static_cast<bool>(face));

  std::vector<Interest> recInterests;
  std::vector<Data>     recDatas;

  face->onFail.connect([] (const std::string& reason) { BOOST_FAIL(reason); });
  face->onReceiveInterest.connect(
      [&recInterests] (const Interest& i) { recInterests.push_back(i); });
  face->onReceiveData.connect([&recDatas] (const Data& d) { recDatas.push_back(d); });

  // check that packet data is not accessed if pcap didn't capture anything (caplen == 0)
  static const pcap_pkthdr header1{};
  face->processIncomingPacket(&header1, nullptr);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // runt frame (too short)
  static const pcap_pkthdr header2{{}, ethernet::HDR_LEN + 6};
  static const uint8_t packet2[ethernet::HDR_LEN + 6]{};
  face->processIncomingPacket(&header2, packet2);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, but TLV block has invalid length
  static const pcap_pkthdr header3{{}, ethernet::HDR_LEN + ethernet::MIN_DATA_LEN};
  static const uint8_t packet3[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,       // NDN ethertype
    tlv::NdnlpData,   // TLV type
    0xfd, 0xff, 0xff  // TLV length (invalid because greater than buffer size)
  };
  face->processIncomingPacket(&header3, packet3);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 0);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, but TLV block has invalid type
  static const pcap_pkthdr header4{{}, ethernet::HDR_LEN + ethernet::MIN_DATA_LEN};
  static const uint8_t packet4[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,       // NDN ethertype
    0x00,             // TLV type (invalid)
    0x00              // TLV length
  };
  face->processIncomingPacket(&header4, packet4);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 2);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame and valid NDNLP header, but invalid payload
  static const pcap_pkthdr header5{{}, ethernet::HDR_LEN + ethernet::MIN_DATA_LEN};
  static const uint8_t packet5[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,                   // NDN ethertype
    tlv::NdnlpData,     0x0e,     // NDNLP header
    tlv::NdnlpSequence, 0x08,
    0, 0, 0, 0, 0, 0, 0, 0,
    tlv::NdnlpPayload,  0x02,
    0x00,             // NDN TLV type (invalid)
    0x00              // NDN TLV length
  };
  face->processIncomingPacket(&header5, packet5);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 18);
  BOOST_CHECK_EQUAL(recInterests.size(), 0);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);

  // valid frame, valid NDNLP header, and valid NDN (interest) packet
  static const pcap_pkthdr header6{{}, ethernet::HDR_LEN + ethernet::MIN_DATA_LEN};
  static const uint8_t packet6[ethernet::HDR_LEN + ethernet::MIN_DATA_LEN]{
    0x01, 0x00, 0x5e, 0x00, 0x17, 0xaa, // destination address
    0x02, 0x00, 0x00, 0x00, 0x00, 0x02, // source address
    0x86, 0x24,                         // NDN ethertype
    tlv::NdnlpData, 0x24,               // NDNLP TLV type and length
    0x51, 0x08, 0x00, 0x00, 0x00, 0x00, // rest of NDNLP header
    0x00, 0x00, 0x00, 0x00, 0x54, 0x18,
    tlv::Interest, 0x16,                // NDN TLV type and length
    0x07, 0x0e, 0x08, 0x07, 0x65, 0x78, // payload
    0x61, 0x6d, 0x70, 0x6c, 0x65, 0x08,
    0x03, 0x66, 0x6f, 0x6f, 0x0a, 0x04,
    0x03, 0xef, 0xe9, 0x7c
  };
  face->processIncomingPacket(&header6, packet6);
  BOOST_CHECK_EQUAL(face->getCounters().getNInBytes(), 56);
  BOOST_CHECK_EQUAL(recInterests.size(), 1);
  BOOST_CHECK_EQUAL(recDatas.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
