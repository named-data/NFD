/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "topology-tester.hpp"
#include <ndn-cxx/encoding/encoding-buffer-fwd.hpp>
#include "face/generic-link-service.hpp"

namespace nfd {
namespace fw {
namespace tests {

using face::InternalTransportBase;
using face::InternalForwarderTransport;
using face::InternalClientTransport;
using face::GenericLinkService;

TopologyLink::TopologyLink(time::nanoseconds delay)
  : m_isUp(true)
{
  this->setDelay(delay);
}

void
TopologyLink::block(TopologyNode i, TopologyNode j)
{
  m_transports.at(i).blockedDestinations.insert(j);
}

void
TopologyLink::unblock(TopologyNode i, TopologyNode j)
{
  m_transports.at(i).blockedDestinations.erase(j);
}

void
TopologyLink::setDelay(time::nanoseconds delay)
{
  BOOST_ASSERT(delay > time::nanoseconds::zero()); // zero delay does not work on macOS
  m_delay = delay;
}

void
TopologyLink::addFace(TopologyNode i, shared_ptr<Face> face)
{
  BOOST_ASSERT(m_transports.count(i) == 0);
  auto& nodeTransport = m_transports[i];

  nodeTransport.face = face;

  nodeTransport.transport = dynamic_cast<InternalTransportBase*>(face->getTransport());
  BOOST_ASSERT(nodeTransport.transport != nullptr);
  nodeTransport.transport->afterSend.connect(
    [this, i] (const Block& packet) { this->transmit(i, packet); });
}

void
TopologyLink::transmit(TopologyNode i, const Block& packet)
{
  if (!m_isUp) {
    return;
  }

  const auto& blockedDestinations = m_transports.at(i).blockedDestinations;

  for (const auto& p : m_transports) {
    if (p.first == i || blockedDestinations.count(p.first) > 0) {
      continue;
    }

    InternalTransportBase* recipient = p.second.transport;
    this->scheduleReceive(recipient, packet);
  }
}

void
TopologyLink::scheduleReceive(InternalTransportBase* recipient, const Block& packet)
{
  scheduler::schedule(m_delay, [packet, recipient] {
    recipient->receiveFromLink(packet);
  });
}

TopologyAppLink::TopologyAppLink(shared_ptr<Face> forwarderFace)
  : m_face(forwarderFace)
  , m_forwarderTransport(static_cast<InternalForwarderTransport*>(forwarderFace->getTransport()))
  , m_clientTransport(make_shared<InternalClientTransport>())
  , m_client(make_shared<ndn::Face>(m_clientTransport, getGlobalIoService()))
{
  this->recover();
}

void
TopologyAppLink::fail()
{
  m_clientTransport->connectToForwarder(nullptr);
}

void
TopologyAppLink::recover()
{
  m_clientTransport->connectToForwarder(m_forwarderTransport);
}

class TopologyPcapLinkService : public GenericLinkService
                              , public TopologyPcap
{
private:
  ///\todo #3941 call GenericLinkServiceCounters constructor in TopologyPcapLinkService constructor

  void
  doSendInterest(const Interest& interest) override
  {
    this->sentInterests.push_back(interest);
    this->sentInterests.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendInterest(interest);
  }

  void
  doSendData(const Data& data) override
  {
    this->sentData.push_back(data);
    this->sentData.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendData(data);
  }

  void
  doSendNack(const lp::Nack& nack) override
  {
    this->sentNacks.push_back(nack);
    this->sentNacks.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendNack(nack);
  }
};

TopologyNode
TopologyTester::addForwarder(const std::string& label)
{
  size_t i = m_forwarders.size();
  m_forwarders.push_back(make_unique<Forwarder>());
  m_forwarderLabels.push_back(label);
  BOOST_ASSERT(m_forwarders.size() == m_forwarderLabels.size());
  return i;
}

shared_ptr<TopologyLink>
TopologyTester::addLink(const std::string& label, time::nanoseconds delay,
                        std::initializer_list<TopologyNode> forwarders,
                        ndn::nfd::LinkType linkType)
{
  auto link = std::make_shared<TopologyLink>(delay);
  FaceUri remoteUri("topology://link/" + label);
  if (linkType == ndn::nfd::LINK_TYPE_NONE) {
    linkType = forwarders.size() > 2 ? ndn::nfd::LINK_TYPE_MULTI_ACCESS :
                                       ndn::nfd::LINK_TYPE_POINT_TO_POINT;
  }
  BOOST_ASSERT(forwarders.size() <= 2 || linkType != ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  for (TopologyNode i : forwarders) {
    Forwarder& forwarder = this->getForwarder(i);
    FaceUri localUri("topology://" + m_forwarderLabels.at(i) + "/" + label);

    unique_ptr<GenericLinkService> service = m_wantPcap ? make_unique<TopologyPcapLinkService>() :
                                                          make_unique<GenericLinkService>();
    auto transport = make_unique<InternalForwarderTransport>(localUri, remoteUri,
                     ndn::nfd::FACE_SCOPE_NON_LOCAL, linkType);
    auto face = make_shared<Face>(std::move(service), std::move(transport));

    forwarder.addFace(face);
    link->addFace(i, face);
  }

  m_links.push_back(link); // keep a shared_ptr so callers don't have to
  return link;
}

shared_ptr<TopologyAppLink>
TopologyTester::addAppFace(const std::string& label, TopologyNode i)
{
  Forwarder& forwarder = this->getForwarder(i);
  FaceUri localUri("topology://" + m_forwarderLabels.at(i) + "/local/" + label);
  FaceUri remoteUri("topology://" + m_forwarderLabels.at(i) + "/app/" + label);

  unique_ptr<GenericLinkService> service = m_wantPcap ? make_unique<TopologyPcapLinkService>() :
                                                        make_unique<GenericLinkService>();
  auto transport = make_unique<InternalForwarderTransport>(localUri, remoteUri,
                   ndn::nfd::FACE_SCOPE_LOCAL, ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  auto face = make_shared<Face>(std::move(service), std::move(transport));

  forwarder.addFace(face);

  auto al = make_shared<TopologyAppLink>(face);
  m_appLinks.push_back(al); // keep a shared_ptr so callers don't have to
  return al;
}

shared_ptr<TopologyAppLink>
TopologyTester::addAppFace(const std::string& label, TopologyNode i, const Name& prefix, uint64_t cost)
{
  shared_ptr<TopologyAppLink> al = this->addAppFace(label, i);
  this->registerPrefix(i, al->getForwarderFace(), prefix, cost);
  return al;
}

void
TopologyTester::enablePcap(bool isEnabled)
{
  m_wantPcap = isEnabled;
}

TopologyPcap&
TopologyTester::getPcap(const Face& face)
{
  return dynamic_cast<TopologyPcapLinkService&>(*face.getLinkService());
}

void
TopologyTester::registerPrefix(TopologyNode i, const Face& face, const Name& prefix, uint64_t cost)
{
  Forwarder& forwarder = this->getForwarder(i);
  Fib& fib = forwarder.getFib();
  fib.insert(prefix).first->addNextHop(const_cast<Face&>(face), cost);
}

void
TopologyTester::addEchoProducer(ndn::Face& face, const Name& prefix)
{
  face.setInterestFilter(prefix,
      [&face] (const ndn::InterestFilter&, const Interest& interest) {
        shared_ptr<Data> data = makeData(interest.getName());
        face.put(*data);
      });
}

void
TopologyTester::addIntervalConsumer(ndn::Face& face, const Name& prefix,
                                    time::nanoseconds interval, size_t n, int seq)
{
  Name name(prefix);
  if (seq >= 0) {
    name.appendSequenceNumber(seq);
    ++seq;
  }
  else {
    name.appendTimestamp();
  }

  shared_ptr<Interest> interest = makeInterest(name);
  face.expressInterest(*interest, nullptr, nullptr, nullptr);

  if (n > 1) {
    scheduler::schedule(interval, bind(&TopologyTester::addIntervalConsumer, this,
                                       ref(face), prefix, interval, n - 1, seq));
  }
}

} // namespace tests
} // namespace fw
} // namespace nfd
