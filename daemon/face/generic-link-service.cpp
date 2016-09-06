/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "generic-link-service.hpp"
#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace face {

NFD_LOG_INIT("GenericLinkService");

GenericLinkServiceCounters::GenericLinkServiceCounters(const LpReassembler& reassembler)
  : nReassembling(reassembler)
{
}

GenericLinkService::Options::Options()
  : allowLocalFields(false)
  , allowFragmentation(false)
  , allowReassembly(false)
{
}

GenericLinkService::GenericLinkService(const GenericLinkService::Options& options)
  : GenericLinkServiceCounters(m_reassembler)
  , m_options(options)
  , m_fragmenter(m_options.fragmenterOptions, this)
  , m_reassembler(m_options.reassemblerOptions, this)
  , m_lastSeqNo(-2)
{
  m_reassembler.beforeTimeout.connect(bind([this] { ++this->nReassemblyTimeouts; }));
}

void
GenericLinkService::doSendInterest(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());

  if (m_options.allowLocalFields) {
    encodeLocalFields(interest, lpPacket);
  }

  this->sendNetPacket(std::move(lpPacket));
}

void
GenericLinkService::doSendData(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());

  if (m_options.allowLocalFields) {
    encodeLocalFields(data, lpPacket);
  }

  this->sendNetPacket(std::move(lpPacket));
}

void
GenericLinkService::doSendNack(const lp::Nack& nack)
{
  lp::Packet lpPacket(nack.getInterest().wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());

  if (m_options.allowLocalFields) {
    encodeLocalFields(nack, lpPacket);
  }

  this->sendNetPacket(std::move(lpPacket));
}

void
GenericLinkService::encodeLocalFields(const ndn::TagHost& netPkt, lp::Packet& lpPacket)
{
  shared_ptr<lp::IncomingFaceIdTag> incomingFaceIdTag = netPkt.getTag<lp::IncomingFaceIdTag>();
  if (incomingFaceIdTag != nullptr) {
    lpPacket.add<lp::IncomingFaceIdField>(*incomingFaceIdTag);
  }
}

void
GenericLinkService::sendNetPacket(lp::Packet&& pkt)
{
  std::vector<lp::Packet> frags;
  const ssize_t mtu = this->getTransport()->getMtu();
  if (m_options.allowFragmentation && mtu != MTU_UNLIMITED) {
    bool isOk = false;
    std::tie(isOk, frags) = m_fragmenter.fragmentPacket(pkt, mtu);
    if (!isOk) {
      // fragmentation failed (warning is logged by LpFragmenter)
      ++this->nFragmentationErrors;
      return;
    }
  }
  else {
    frags.push_back(pkt);
  }

  if (frags.size() > 1) {
    // sequence is needed only if packet is fragmented
    this->assignSequences(frags);
  }
  else {
    // even if indexed fragmentation is enabled, the fragmenter should not
    // fragment the packet if it can fit in MTU
    BOOST_ASSERT(frags.size() > 0);
    BOOST_ASSERT(!frags.front().has<lp::FragIndexField>());
    BOOST_ASSERT(!frags.front().has<lp::FragCountField>());
  }

  for (const lp::Packet& frag : frags) {
    Transport::Packet tp(frag.wireEncode());
    if (mtu != MTU_UNLIMITED && tp.packet.size() > static_cast<size_t>(mtu)) {
      ++this->nOutOverMtu;
      NFD_LOG_FACE_WARN("attempt to send packet over MTU limit");
      continue;
    }
    this->sendPacket(std::move(tp));
  }
}

void
GenericLinkService::assignSequence(lp::Packet& pkt)
{
  pkt.set<lp::SequenceField>(++m_lastSeqNo);
}

void
GenericLinkService::assignSequences(std::vector<lp::Packet>& pkts)
{
  std::for_each(pkts.begin(), pkts.end(), bind(&GenericLinkService::assignSequence, this, _1));
}

void
GenericLinkService::doReceivePacket(Transport::Packet&& packet)
{
  try {
    lp::Packet pkt(packet.packet);

    if (!pkt.has<lp::FragmentField>()) {
      NFD_LOG_FACE_TRACE("received IDLE packet: DROP");
      return;
    }

    if ((pkt.has<lp::FragIndexField>() || pkt.has<lp::FragCountField>()) &&
        !m_options.allowReassembly) {
      NFD_LOG_FACE_WARN("received fragment, but reassembly disabled: DROP");
      return;
    }

    bool isReassembled = false;
    Block netPkt;
    lp::Packet firstPkt;
    std::tie(isReassembled, netPkt, firstPkt) = m_reassembler.receiveFragment(packet.remoteEndpoint,
                                                                              pkt);
    if (isReassembled) {
      this->decodeNetPacket(netPkt, firstPkt);
    }
  }
  catch (const tlv::Error& e) {
    ++this->nInLpInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeNetPacket(const Block& netPkt, const lp::Packet& firstPkt)
{
  try {
    switch (netPkt.type()) {
      case tlv::Interest:
        if (firstPkt.has<lp::NackField>()) {
          this->decodeNack(netPkt, firstPkt);
        }
        else {
          this->decodeInterest(netPkt, firstPkt);
        }
        break;
      case tlv::Data:
        this->decodeData(netPkt, firstPkt);
        break;
      default:
        ++this->nInNetInvalid;
        NFD_LOG_FACE_WARN("unrecognized network-layer packet TLV-TYPE " << netPkt.type() << ": DROP");
        return;
    }
  }
  catch (const tlv::Error& e) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("packet parse error (" << e.what() << "): DROP");
  }
}

void
GenericLinkService::decodeInterest(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(!firstPkt.has<lp::NackField>());

  // forwarding expects Interest to be created with make_shared
  auto interest = make_shared<Interest>(netPkt);

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    if (m_options.allowLocalFields) {
      interest->setTag(make_shared<lp::NextHopFaceIdTag>(firstPkt.get<lp::NextHopFaceIdField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received NextHopFaceId, but local fields disabled: DROP");
      return;
    }
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Interest: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  this->receiveInterest(*interest);
}

void
GenericLinkService::decodeData(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Data);

  // forwarding expects Data to be created with make_shared
  auto data = make_shared<Data>(netPkt);

  if (firstPkt.has<lp::NackField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received Nack with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    if (m_options.allowLocalFields) {
      // In case of an invalid CachePolicyType, get<lp::CachePolicyField> will throw,
      // so it's unnecessary to check here.
      data->setTag(make_shared<lp::CachePolicyTag>(firstPkt.get<lp::CachePolicyField>()));
    }
    else {
      NFD_LOG_FACE_WARN("received CachePolicy, but local fields disabled: IGNORE");
    }
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  this->receiveData(*data);
}

void
GenericLinkService::decodeNack(const Block& netPkt, const lp::Packet& firstPkt)
{
  BOOST_ASSERT(netPkt.type() == tlv::Interest);
  BOOST_ASSERT(firstPkt.has<lp::NackField>());

  lp::Nack nack((Interest(netPkt)));
  nack.setHeader(firstPkt.get<lp::NackField>());

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received NextHopFaceId with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    ++this->nInNetInvalid;
    NFD_LOG_FACE_WARN("received CachePolicy with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::IncomingFaceIdField>()) {
    NFD_LOG_FACE_WARN("received IncomingFaceId: IGNORE");
  }

  this->receiveNack(nack);
}

} // namespace face
} // namespace nfd
