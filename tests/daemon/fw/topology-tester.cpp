/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "common/global.hpp"
#include "face/generic-link-service.hpp"

#include <ndn-cxx/encoding/encoding-buffer-fwd.hpp>

namespace nfd {
namespace fw {
namespace tests {

using face::GenericLinkService;
using face::InternalClientTransport;
using face::InternalForwarderTransport;

TopologyLink::NodeTransport::NodeTransport(shared_ptr<Face> f, ReceiveProxy::Callback cb)
  : face(std::move(f))
  , transport(dynamic_cast<InternalForwarderTransport*>(face->getTransport()))
  , proxy(std::move(cb))
{
  BOOST_ASSERT(transport != nullptr);
}

TopologyLink::TopologyLink(time::nanoseconds delay)
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
  auto receiveCb = [this, i] (const Block& packet) { transmit(i, packet); };

  auto ret = m_transports.emplace(std::piecewise_construct,
                                  std::forward_as_tuple(i),
                                  std::forward_as_tuple(std::move(face), std::move(receiveCb)));
  BOOST_ASSERT(ret.second);

  auto& node = ret.first->second;
  node.transport->setPeer(&node.proxy);
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

    getScheduler().schedule(m_delay, [packet, recipient = p.second.transport] {
      recipient->receivePacket(packet);
    });
  }
}

TopologySingleLink::TopologySingleLink(shared_ptr<Face> forwarderFace)
  : m_face(std::move(forwarderFace))
  , m_forwarderTransport(static_cast<InternalForwarderTransport*>(m_face->getTransport()))
{
}

TopologyAppLink::TopologyAppLink(shared_ptr<Face> forwarderFace)
  : TopologySingleLink(std::move(forwarderFace))
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

class TopologyBareLink::Observer : public face::InternalTransportBase
{
public:
  explicit
  Observer(std::vector<Block>& receivedPackets)
    : m_receivedPackets(receivedPackets)
  {
  }

  void
  receivePacket(const Block& packet) final
  {
    m_receivedPackets.push_back(packet);
  }

private:
  std::vector<Block>& m_receivedPackets;
};

TopologyBareLink::TopologyBareLink(shared_ptr<Face> forwarderFace)
  : TopologySingleLink(std::move(forwarderFace))
  , m_observer(make_unique<Observer>(sentPackets))
{
  m_forwarderTransport->setPeer(m_observer.get());
}

void
TopologyBareLink::receivePacket(const Block& packet)
{
  m_forwarderTransport->receivePacket(packet);
}

class TopologyPcapLinkService : public GenericLinkService
                              , public TopologyPcap
{
private:
  ///\todo #3941 call GenericLinkServiceCounters constructor in TopologyPcapLinkService constructor

  void
  doSendInterest(const Interest& interest, const EndpointId& endpointId) override
  {
    this->sentInterests.push_back(interest);
    this->sentInterests.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendInterest(interest, endpointId);
  }

  void
  doSendData(const Data& data, const EndpointId& endpointId) override
  {
    this->sentData.push_back(data);
    this->sentData.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendData(data, endpointId);
  }

  void
  doSendNack(const lp::Nack& nack, const EndpointId& endpointId) override
  {
    this->sentNacks.push_back(nack);
    this->sentNacks.back().setTag(std::make_shared<TopologyPcapTimestamp>(time::steady_clock::now()));
    this->GenericLinkService::doSendNack(nack, endpointId);
  }
};

TopologyNode
TopologyTester::addForwarder(const std::string& label)
{
  size_t i = m_forwarders.size();
  m_forwarders.push_back(make_unique<TopologyForwarder>(label));
  return i;
}

shared_ptr<Face>
TopologyTester::makeFace(TopologyNode i, const FaceUri& localUri, const FaceUri& remoteUri,
                         ndn::nfd::FaceScope scope, ndn::nfd::LinkType linkType)
{
  unique_ptr<GenericLinkService> service = m_wantPcap ? make_unique<TopologyPcapLinkService>() :
                                                        make_unique<GenericLinkService>();
  auto transport = make_unique<InternalForwarderTransport>(localUri, remoteUri, scope, linkType);
  auto face = make_shared<Face>(std::move(service), std::move(transport));
  m_forwarders.at(i)->faceTable.add(face);
  return face;
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
    FaceUri localUri("topology://" + m_forwarders.at(i)->label + "/" + label);
    auto face = makeFace(i, localUri, remoteUri, ndn::nfd::FACE_SCOPE_NON_LOCAL, linkType);
    link->addFace(i, std::move(face));
  }

  m_links.push_back(link); // keep a shared_ptr so callers don't have to
  return link;
}

shared_ptr<TopologyAppLink>
TopologyTester::addAppFace(const std::string& label, TopologyNode i)
{
  FaceUri localUri("topology://" + m_forwarders.at(i)->label + "/local/" + label);
  FaceUri remoteUri("topology://" + m_forwarders.at(i)->label + "/app/" + label);
  auto face = makeFace(i, localUri, remoteUri, ndn::nfd::FACE_SCOPE_LOCAL, ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  auto al = make_shared<TopologyAppLink>(std::move(face));
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

shared_ptr<TopologyBareLink>
TopologyTester::addBareLink(const std::string& label, TopologyNode i, ndn::nfd::FaceScope scope,
                            ndn::nfd::LinkType linkType)
{
  FaceUri localUri("topology://" + m_forwarders.at(i)->label + "/local/" + label);
  FaceUri remoteUri("topology://" + m_forwarders.at(i)->label + "/bare/" + label);
  auto face = makeFace(i, localUri, remoteUri, scope, linkType);

  auto bl = make_shared<TopologyBareLink>(std::move(face));
  m_bareLinks.push_back(bl); // keep a shared_ptr so callers don't have to
  return bl;
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
  fib::Entry* entry = fib.insert(prefix).first;
  fib.addOrUpdateNextHop(*entry, const_cast<Face&>(face), cost);
}

void
TopologyTester::addEchoProducer(ndn::Face& face, const Name& prefix, time::nanoseconds replyDelay)
{
  BOOST_ASSERT(replyDelay >= 0_ns);

  face.setInterestFilter(prefix, [=, &face] (const auto&, const auto& interest) {
    auto data = makeData(interest.getName());
    if (replyDelay == 0_ns) {
      // reply immediately
      face.put(*data);
    }
    else {
      // delay the reply
      getScheduler().schedule(replyDelay, [&face, data = std::move(data)] {
        face.put(*data);
      });
    }
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

  auto interest = makeInterest(name);
  face.expressInterest(*interest, nullptr, nullptr, nullptr);

  if (n > 1) {
    getScheduler().schedule(interval, [=, &face] {
      addIntervalConsumer(face, prefix, interval, n - 1, seq);
    });
  }
}

} // namespace tests
} // namespace fw
} // namespace nfd
