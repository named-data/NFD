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

void
GenericLinkService::doSendInterest(const Interest& interest)
{
  lp::Packet lpPacket(interest.wireEncode());
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

void
GenericLinkService::doSendData(const Data& data)
{
  lp::Packet lpPacket(data.wireEncode());
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

void
GenericLinkService::doSendNack(const lp::Nack& nack)
{
  lp::Packet lpPacket(nack.getInterest().wireEncode());
  lpPacket.add<lp::NackField>(nack.getHeader());
  Transport::Packet packet;
  packet.packet = lpPacket.wireEncode();
  sendPacket(std::move(packet));
}

void
GenericLinkService::doReceivePacket(Transport::Packet&& packet)
{
  lp::Packet lpPacket(packet.packet);
  ndn::Buffer::const_iterator fragBegin, fragEnd;
  std::tie(fragBegin, fragEnd) = lpPacket.get<lp::FragmentField>();
  Block netPacket(&*fragBegin, std::distance(fragBegin, fragEnd));

  // Forwarding expects Interest and Data to be created with make_shared,
  // but has no such requirement on Nack.
  switch (netPacket.type()) {
    case tlv::Interest: {
      auto interest = make_shared<Interest>(netPacket);
      if (lpPacket.has<lp::NackField>()) {
        lp::Nack nack(std::move(*interest));
        nack.setHeader(lpPacket.get<lp::NackField>());
        receiveNack(nack);
      }
      else {
        receiveInterest(*interest);
      }
      break;
    }
    case tlv::Data: {
      auto data = make_shared<Data>(netPacket);
      receiveData(*data);
      break;
    }
  }
}

} // namespace face
} // namespace nfd
