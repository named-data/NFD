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

#include "face/ethernet-factory.hpp"
#include "core/network-interface.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceEthernet, BaseFixture)

BOOST_AUTO_TEST_CASE(GetChannels)
{
  EthernetFactory factory;

  auto channels = factory.getChannels();
  BOOST_CHECK_EQUAL(channels.empty(), true);
}

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


BOOST_FIXTURE_TEST_CASE(MulticastFacesMap, InterfacesFixture)
{
  EthernetFactory factory;

  if (m_interfaces.empty())
    {
      BOOST_WARN_MESSAGE(false, "No interfaces available, cannot perform MulticastFacesMap test");
      return;
    }

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
    BOOST_WARN_MESSAGE(false, "Cannot test second EthernetFace creation, "
                       "only one interface available");
  }

  shared_ptr<EthernetFace> face3 = factory.createMulticastFace(m_interfaces.front(),
                                     ethernet::getDefaultMulticastAddress());
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
                    "ether://[" + ethernet::getDefaultMulticastAddress().toString() + "]");
  BOOST_CHECK_EQUAL(face->getLocalUri().toString(),
                    "dev://" + m_interfaces.front().name);

  shared_ptr<Interest> interest1 = makeInterest("ndn:/TpnzGvW9R");
  shared_ptr<Data>     data1     = makeData("ndn:/KfczhUqVix");
  shared_ptr<Interest> interest2 = makeInterest("ndn:/QWiIMfj5sL");
  shared_ptr<Data>     data2     = makeData("ndn:/XNBV796f");

  BOOST_CHECK_NO_THROW(face->sendInterest(*interest1));
  BOOST_CHECK_NO_THROW(face->sendData    (*data1    ));
  BOOST_CHECK_NO_THROW(face->sendInterest(*interest2));
  BOOST_CHECK_NO_THROW(face->sendData    (*data2    ));

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
