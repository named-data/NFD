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

#include "internal-face.hpp"

namespace nfd {

InternalFace::InternalFace()
  : Face(FaceUri("internal://"), FaceUri("internal://"), true)
{
}

void
InternalFace::sendInterest(const Interest& interest)
{
  this->emitSignal(onSendInterest, interest);
}

void
InternalFace::sendData(const Data& data)
{
  this->emitSignal(onSendData, data);
}

void
InternalFace::close()
{
}

void
InternalFace::receive(const Block& blockFromClient)
{
  const Block& payLoad = ndn::nfd::LocalControlHeader::getPayload(blockFromClient);
  if (payLoad.type() == ndn::tlv::Interest) {
    receiveInterest(*extractPacketFromBlock<Interest>(blockFromClient, payLoad));
  }
  else if (payLoad.type() == ndn::tlv::Data) {
    receiveData(*extractPacketFromBlock<Data>(blockFromClient, payLoad));
  }
}

void
InternalFace::receiveInterest(const Interest& interest)
{
  this->emitSignal(onReceiveInterest, interest);
}

void
InternalFace::receiveData(const Data& data)
{
  this->emitSignal(onReceiveData, data);
}

template<typename Packet>
shared_ptr<Packet>
InternalFace::extractPacketFromBlock(const Block& blockFromClient, const Block& payLoad)
{
  auto packet = make_shared<Packet>(payLoad);
  if (&payLoad != &blockFromClient) {
    packet->getLocalControlHeader().wireDecode(blockFromClient);
  }
  return packet;
}

template shared_ptr<Interest>
InternalFace::extractPacketFromBlock<Interest>(const Block& blockFromClient, const Block& payLoad);

template shared_ptr<Data>
InternalFace::extractPacketFromBlock<Data>(const Block& blockFromClient, const Block& payLoad);

} // namespace nfd
