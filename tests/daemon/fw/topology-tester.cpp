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

#include "topology-tester.hpp"
#include <ndn-cxx/encoding/encoding-buffer-fwd.hpp>
#include "face/generic-link-service.hpp"

namespace nfd {
namespace fw {
namespace tests {

using face::LpFaceWrapper;

TopologyForwarderTransport::TopologyForwarderTransport(
    const FaceUri& localUri, const FaceUri& remoteUri,
    ndn::nfd::FaceScope scope, ndn::nfd::LinkType linkType)
{
  this->setLocalUri(localUri);
  this->setRemoteUri(remoteUri);
  this->setScope(scope);
  this->setPersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT);
  this->setLinkType(linkType);
}

void
TopologyForwarderTransport::receiveFromTopology(const Block& packet)
{
  Packet p;
  p.packet = packet;
  this->receive(std::move(p));
}

void
TopologyForwarderTransport::doSend(Packet&& packet)
{
  this->emitSignal(afterSend, packet.packet);
}

void
TopologyClientTransport::receiveFromTopology(const Block& packet)
{
  if (m_receiveCallback) {
    m_receiveCallback(packet);
  }
}

void
TopologyClientTransport::send(const Block& wire)
{
  this->emitSignal(afterSend, wire);
}

void
TopologyClientTransport::send(const Block& header, const Block& payload)
{
  ndn::EncodingBuffer encoder(header.size() + payload.size(), header.size() + payload.size());
  encoder.appendByteArray(header.wire(), header.size());
  encoder.appendByteArray(payload.wire(), payload.size());

  this->send(encoder.block());
}

TopologyLinkBase::TopologyLinkBase()
  : m_isUp(true)
{
}

void
TopologyLinkBase::attachTransport(TopologyNode i, TopologyTransportBase* transport)
{
  BOOST_ASSERT(transport != nullptr);
  BOOST_ASSERT(m_transports.count(i) == 0);

  m_transports[i] = transport;
  transport->afterSend.connect([this, i] (const Block& packet) { this->transmit(i, packet); });
}

void
TopologyLinkBase::transmit(TopologyNode i, const Block& packet)
{
  if (!m_isUp) {
    return;
  }

  for (auto&& p : m_transports) {
    if (p.first == i) {
      continue;
    }

    TopologyTransportBase* recipient = p.second;
    this->scheduleReceive(recipient, packet);
  }
}

TopologyLink::TopologyLink(const time::nanoseconds& delay)
  : m_delay(delay)
{
  BOOST_ASSERT(delay > time::nanoseconds::zero());
  // zero delay does not work on OSX
}

void
TopologyLink::addFace(TopologyNode i, shared_ptr<LpFaceWrapper> face)
{
  this->attachTransport(i, dynamic_cast<TopologyTransportBase*>(face->getLpFace()->getTransport()));
  m_faces[i] = face;
}

void
TopologyLink::scheduleReceive(TopologyTransportBase* recipient, const Block& packet)
{
  scheduler::schedule(m_delay, [packet, recipient] {
    recipient->receiveFromTopology(packet);
  });
}

TopologyAppLink::TopologyAppLink(shared_ptr<LpFaceWrapper> face)
  : m_face(face)
{
  this->attachTransport(0, dynamic_cast<TopologyTransportBase*>(face->getLpFace()->getTransport()));

  auto clientTransport = make_shared<TopologyClientTransport>();
  m_client = make_shared<ndn::Face>(clientTransport, getGlobalIoService());
  this->attachTransport(1, clientTransport.get());
}

void
TopologyAppLink::scheduleReceive(TopologyTransportBase* recipient, const Block& packet)
{
  getGlobalIoService().post([packet, recipient] {
    recipient->receiveFromTopology(packet);
  });
}

TopologyNode
TopologyTester::addForwarder(const std::string& label)
{
  size_t i = m_forwarders.size();
  m_forwarders.push_back(std::move(make_unique<Forwarder>()));
  m_forwarderLabels.push_back(label);
  BOOST_ASSERT(m_forwarders.size() == m_forwarderLabels.size());
  return i;
}

shared_ptr<TopologyLink>
TopologyTester::addLink(const std::string& label, const time::nanoseconds& delay,
                        std::initializer_list<TopologyNode> forwarders,
                        bool forceMultiAccessFace)
{
  auto link = make_shared<TopologyLink>(delay);
  FaceUri remoteUri("topology://link/" + label);
  ndn::nfd::LinkType linkType = (forceMultiAccessFace || forwarders.size() > 2) ?
                                ndn::nfd::LINK_TYPE_MULTI_ACCESS :
                                ndn::nfd::LINK_TYPE_POINT_TO_POINT;

  for (TopologyNode i : forwarders) {
    Forwarder& forwarder = this->getForwarder(i);
    FaceUri localUri("topology://" + m_forwarderLabels.at(i) + "/" + label);

    auto service = make_unique<face::GenericLinkService>();
    auto transport = make_unique<TopologyForwarderTransport>(localUri, remoteUri,
                     ndn::nfd::FACE_SCOPE_NON_LOCAL, linkType);
    auto face = make_unique<LpFace>(std::move(service), std::move(transport));
    auto faceW = make_shared<LpFaceWrapper>(std::move(face));

    forwarder.addFace(faceW);
    link->addFace(i, faceW);
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

  auto service = make_unique<face::GenericLinkService>();
  auto transport = make_unique<TopologyForwarderTransport>(localUri, remoteUri,
                   ndn::nfd::FACE_SCOPE_LOCAL, ndn::nfd::LINK_TYPE_POINT_TO_POINT);
  auto face = make_unique<LpFace>(std::move(service), std::move(transport));
  auto faceW = make_shared<LpFaceWrapper>(std::move(face));

  forwarder.addFace(faceW);

  auto al = make_shared<TopologyAppLink>(faceW);
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
TopologyTester::registerPrefix(TopologyNode i, const Face& face, const Name& prefix, uint64_t cost)
{
  Forwarder& forwarder = this->getForwarder(i);
  Fib& fib = forwarder.getFib();
  shared_ptr<fib::Entry> fibEntry = fib.insert(prefix).first;
  fibEntry->addNextHop(const_cast<Face&>(face).shared_from_this(), cost);
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
                                    const time::nanoseconds& interval, size_t n)
{
  Name name(prefix);
  name.appendTimestamp();
  shared_ptr<Interest> interest = makeInterest(name);
  face.expressInterest(*interest, bind([]{}));

  if (n > 1) {
    scheduler::schedule(interval, bind(&TopologyTester::addIntervalConsumer, this,
                                       ref(face), prefix, interval, n - 1));
  }
}

} // namespace tests
} // namespace fw
} // namespace nfd
