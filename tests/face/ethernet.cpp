/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/ethernet-channel-factory.hpp"

#include <ndn-cpp-dev/security/key-chain.hpp>

#include <boost/test/unit_test.hpp>

namespace nfd {

BOOST_AUTO_TEST_SUITE(FaceEthernet)

BOOST_AUTO_TEST_CASE(MulticastFacesMap)
{
  EthernetChannelFactory factory;

  std::vector<ethernet::Endpoint> interfaces = EthernetChannelFactory::findAllInterfaces();
  if (interfaces.size() > 0)
    {
      shared_ptr<EthernetFace> face1 = factory.createMulticast(interfaces[0],
                                                               ethernet::getBroadcastAddress());
      shared_ptr<EthernetFace> face1bis = factory.createMulticast(interfaces[0],
                                                                  ethernet::getBroadcastAddress());
      BOOST_CHECK_EQUAL(face1, face1bis);

      if (interfaces.size() > 1)
        {
          shared_ptr<EthernetFace> face2 = factory.createMulticast(interfaces[1],
                                                                   ethernet::getBroadcastAddress());
          BOOST_CHECK_NE(face1, face2);
        }
      else
        {
          BOOST_WARN_MESSAGE(interfaces.size() < 2, "Cannot test second EthernetFace creation, "
                             "only one interface available for pcap");
        }

      shared_ptr<EthernetFace> face3 = factory.createMulticast(interfaces[0],
                                                               ethernet::getDefaultMulticastAddress());
      BOOST_CHECK_NE(face1, face3);
    }
  else
    {
      BOOST_WARN_MESSAGE(interfaces.empty(), "Cannot perform MulticastFacesMap tests, "
                         "no interfaces are available for pcap");
    }
}

BOOST_AUTO_TEST_CASE(SendPacket)
{
  EthernetChannelFactory factory;

  std::vector<ethernet::Endpoint> interfaces = EthernetChannelFactory::findAllInterfaces();
  if (interfaces.empty())
    {
      BOOST_WARN_MESSAGE(interfaces.empty(),
                         "No interfaces are available for pcap, cannot perform SendPacket test");
      return;
    }

  shared_ptr<EthernetFace> face =
    factory.createMulticast(interfaces[0], ethernet::getDefaultMulticastAddress());

  BOOST_REQUIRE(static_cast<bool>(face));
  BOOST_CHECK_EQUAL(face->isLocal(), false);

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

  BOOST_CHECK_NO_THROW(face->sendInterest(interest1));
  BOOST_CHECK_NO_THROW(face->sendData    (data1    ));
  BOOST_CHECK_NO_THROW(face->sendInterest(interest2));
  BOOST_CHECK_NO_THROW(face->sendData    (data2    ));

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

BOOST_AUTO_TEST_SUITE_END()

} // namespace nfd
