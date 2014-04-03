/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face/ethernet-factory.hpp"
#include "core/network-interface.hpp"
#include "tests/test-common.hpp"

#include <ndn-cpp-dev/security/key-chain.hpp>
#include <pcap/pcap.h>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceEthernet, BaseFixture)

class InterfacesFixture : protected BaseFixture
{
protected:
  InterfacesFixture()
  {
    std::list< shared_ptr<NetworkInterfaceInfo> > ifs = listNetworkInterfaces();
    for (std::list< shared_ptr<NetworkInterfaceInfo> >::const_iterator i = ifs.begin();
         i != ifs.end();
         ++i)
      {
        if (!(*i)->isLoopback() && (*i)->isUp())
          {
            pcap_t* p = pcap_create((*i)->name.c_str(), 0);
            if (!p)
              continue;

            if (pcap_activate(p) == 0)
              m_interfaces.push_back(*i);

            pcap_close(p);
          }
      }
  }

protected:
  std::list< shared_ptr<NetworkInterfaceInfo> > m_interfaces;
};


BOOST_FIXTURE_TEST_CASE(MulticastFacesMap, InterfacesFixture)
{
  EthernetFactory factory;

  if (m_interfaces.empty())
    {
      BOOST_WARN_MESSAGE(false, "No interfaces available, cannot perform MulticastFacesMap test");
      return;
    }

  shared_ptr<EthernetFace> face1;
  BOOST_REQUIRE_NO_THROW(face1 = factory.createMulticastFace(m_interfaces.front(),
                                                             ethernet::getBroadcastAddress()));
  shared_ptr<EthernetFace> face1bis;
  BOOST_REQUIRE_NO_THROW(face1bis = factory.createMulticastFace(m_interfaces.front(),
                                                                ethernet::getBroadcastAddress()));
  BOOST_CHECK_EQUAL(face1, face1bis);

  if (m_interfaces.size() > 1)
    {
      shared_ptr<EthernetFace> face2;
      BOOST_REQUIRE_NO_THROW(face2 = factory.createMulticastFace(m_interfaces.back(),
                                                                 ethernet::getBroadcastAddress()));
      BOOST_CHECK_NE(face1, face2);
    }
  else
    {
      BOOST_WARN_MESSAGE(false, "Cannot test second EthernetFace creation, "
                         "only one interface available");
    }

  shared_ptr<EthernetFace> face3;
  BOOST_REQUIRE_NO_THROW(face3 = factory.createMulticastFace(m_interfaces.front(),
                                                             ethernet::getDefaultMulticastAddress()));
  BOOST_CHECK_NE(face1, face3);
}

BOOST_FIXTURE_TEST_CASE(SendPacket, InterfacesFixture)
{
  EthernetFactory factory;

  if (m_interfaces.empty())
    {
      BOOST_WARN_MESSAGE(false, "No interfaces available for pcap, cannot perform SendPacket test");
      return;
    }

  shared_ptr<EthernetFace> face = factory.createMulticastFace(m_interfaces.front(),
                                                              ethernet::getDefaultMulticastAddress());

  BOOST_REQUIRE(static_cast<bool>(face));

  BOOST_CHECK(!face->isOnDemand());
  BOOST_CHECK_EQUAL(face->isLocal(), false);
  BOOST_CHECK_EQUAL(face->getRemoteUri().toString(),
                    "ether://" + ethernet::getDefaultMulticastAddress().toString(':'));
  BOOST_CHECK_EQUAL(face->getLocalUri().toString(),
                    "dev://" + m_interfaces.front()->name);

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

} // namespace tests
} // namespace nfd
