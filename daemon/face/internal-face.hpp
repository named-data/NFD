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

#ifndef NFD_DAEMON_FACE_INTERNAL_FACE_HPP
#define NFD_DAEMON_FACE_INTERNAL_FACE_HPP

#include "face.hpp"

namespace nfd {

/**
 * @brief represents a face for internal use in NFD.
 */
class InternalFace : public Face
{
public:

  InternalFace();

  virtual void
  sendInterest(const Interest& interest) DECL_OVERRIDE;

  virtual void
  sendData(const Data& data) DECL_OVERRIDE;

  virtual void
  close() DECL_OVERRIDE;

  /**
   * @brief receive a block from a client face
   *
   * step1. extracte the packet payload from the received block.
   * step2. check the type (either Interest or Data) through the payload.
   * step3. call receiveInterest / receiveData respectively according to the type.
   *
   * @param blockFromClient the block from a client face
   */
  void
  receive(const Block& blockFromClient);

  /**
   * @brief receive an Interest from a client face
   *
   * emit the onReceiveInterest signal.
   *
   * @param interest the received Interest packet
   */
  void
  receiveInterest(const Interest& interest);

  /**
   * @brief receive a Data from a client face
   *
   * emit the onReceiveData signal.
   *
   * @param data the received Data packet
   */
  void
  receiveData(const Data& data);

  /**
   * @brief compose the whole packet from the received block after payload is extracted
   *
   * construct a packet from the extracted payload, and then decode the localControlHeader if the
   * received block holds more information than the payload.
   *
   * @tparam Packet the type of packet, Interest or Data
   * @param blockFromClient the received block
   * @param payLoad the extracted payload
   *
   * @return a complete packet
   */
  template<typename Packet>
  static shared_ptr<Packet>
  extractPacketFromBlock(const Block& blockFromClient, const Block& payLoad);
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_INTERNAL_FACE_HPP
