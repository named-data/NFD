/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef NFD_DAEMON_FACE_FACE_COUNTERS_HPP
#define NFD_DAEMON_FACE_FACE_COUNTERS_HPP

#include "link-service.hpp"
#include "transport.hpp"

namespace nfd {
namespace face {

/** \brief gives access to counters provided by Face
 *
 *  This type is a facade that exposes common counters of a Face.
 *
 *  get<T>() can be used to access extended counters provided by
 *  LinkService or Transport of the Face.
 */
class FaceCounters
{
public:
  FaceCounters(const LinkService::Counters& linkServiceCounters,
               const Transport::Counters& transportCounters);

  /** \return counters provided by LinkService
   *  \tparam T LinkService counters type
   *  \throw std::bad_cast counters type mismatch
   */
  template<typename T>
  typename std::enable_if<std::is_base_of<LinkService::Counters, T>::value, const T&>::type
  get() const
  {
    return dynamic_cast<const T&>(m_linkServiceCounters);
  }

  /** \return counters provided by Transport
   *  \tparam T Transport counters type
   *  \throw std::bad_cast counters type mismatch
   */
  template<typename T>
  typename std::enable_if<std::is_base_of<Transport::Counters, T>::value, const T&>::type
  get() const
  {
    return dynamic_cast<const T&>(m_transportCounters);
  }

public:
  const PacketCounter& nInInterests;
  const PacketCounter& nOutInterests;
  const PacketCounter& nDroppedInterests;
  const PacketCounter& nInData;
  const PacketCounter& nOutData;
  const PacketCounter& nInNacks;
  const PacketCounter& nOutNacks;

  const PacketCounter& nInPackets;
  const PacketCounter& nOutPackets;
  const ByteCounter& nInBytes;
  const ByteCounter& nOutBytes;

private:
  const LinkService::Counters& m_linkServiceCounters;
  const Transport::Counters& m_transportCounters;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_COUNTERS_HPP
