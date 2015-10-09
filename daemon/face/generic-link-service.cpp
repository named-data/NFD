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

#include "generic-link-service.hpp"

namespace nfd {
namespace face {

NFD_LOG_INIT("GenericLinkService");

GenericLinkService::Options::Options()
  : allowLocalFields(false)
{
}

GenericLinkService::GenericLinkService(const GenericLinkService::Options& options)
  : m_options(options)
{
}

void
GenericLinkService::doSendInterest(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());
  if (m_options.allowLocalFields) {
    encodeLocalFields(interest, lpPacket);
  }
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

void
GenericLinkService::doSendData(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());
  if (m_options.allowLocalFields) {
    encodeLocalFields(data, lpPacket);
  }
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

void
GenericLinkService::doSendNack(const lp::Nack& nack)
{
  lp::Packet lpPacket(nack.getInterest().wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());
  if (m_options.allowLocalFields) {
    encodeLocalFields(nack.getInterest(), lpPacket);
  }
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

bool
GenericLinkService::encodeLocalFields(const Interest& interest, lp::Packet& lpPacket)
{
  if (interest.getLocalControlHeader().hasIncomingFaceId()) {
    lpPacket.add<lp::IncomingFaceIdField>(interest.getIncomingFaceId());
  }

  if (interest.getLocalControlHeader().hasCachingPolicy()) {
    // Packet must be dropped
    return false;
  }

  return true;
}

bool
GenericLinkService::encodeLocalFields(const Data& data, lp::Packet& lpPacket)
{
  if (data.getLocalControlHeader().hasIncomingFaceId()) {
    lpPacket.add<lp::IncomingFaceIdField>(data.getIncomingFaceId());
  }

  if (data.getLocalControlHeader().hasCachingPolicy()) {
    switch (data.getCachingPolicy()) {
      case ndn::nfd::LocalControlHeader::CachingPolicy::NO_CACHE: {
        lp::CachePolicy cachePolicy;
        cachePolicy.setPolicy(lp::CachePolicyType::NO_CACHE);
        lpPacket.add<lp::CachePolicyField>(cachePolicy);
        break;
      }
      default: {
        break;
      }
    }
  }

  return true;
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

    if (pkt.has<lp::FragIndexField>() || pkt.has<lp::FragCountField>()) {
      NFD_LOG_FACE_WARN("received fragment, but reassembly not implemented: DROP");
      return;
    }

    ndn::Buffer::const_iterator fragBegin, fragEnd;
    std::tie(fragBegin, fragEnd) = pkt.get<lp::FragmentField>();
    Block netPkt(&*fragBegin, std::distance(fragBegin, fragEnd));

    switch (netPkt.type()) {
      case tlv::Interest:
        if (pkt.has<lp::NackField>()) {
          this->decodeNack(netPkt, pkt);
        }
        else {
          this->decodeInterest(netPkt, pkt);
        }
        break;
      case tlv::Data:
        this->decodeData(netPkt, pkt);
        break;
      default:
        NFD_LOG_FACE_WARN("unrecognized network-layer packet TLV-TYPE " << netPkt.type() << ": DROP");
        return;
    }
  }
  catch (const tlv::Error& e) {
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
      interest->setNextHopFaceId(firstPkt.get<lp::NextHopFaceIdField>());
    }
    else {
      NFD_LOG_FACE_WARN("received NextHopFaceId, but local fields disabled: DROP");
      return;
    }
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
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
    NFD_LOG_FACE_WARN("received Nack with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::NextHopFaceIdField>()) {
    NFD_LOG_FACE_WARN("received NextHopFaceId with Data: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
    if (m_options.allowLocalFields) {
      lp::CachePolicyType policy = firstPkt.get<lp::CachePolicyField>().getPolicy();
      switch (policy) {
        case lp::CachePolicyType::NO_CACHE:
          data->setCachingPolicy(ndn::nfd::LocalControlHeader::CachingPolicy::NO_CACHE);
          break;
        default:
          NFD_LOG_FACE_WARN("unrecognized CachePolicyType " << policy << ": DROP");
          return;
      }
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
    NFD_LOG_FACE_WARN("received NextHopFaceId with Nack: DROP");
    return;
  }

  if (firstPkt.has<lp::CachePolicyField>()) {
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
