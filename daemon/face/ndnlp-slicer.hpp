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

#ifndef NFD_DAEMON_FACE_NDNLP_SLICER_HPP
#define NFD_DAEMON_FACE_NDNLP_SLICER_HPP

#include "ndnlp-tlv.hpp"
#include "ndnlp-sequence-generator.hpp"

namespace nfd {
namespace ndnlp {

typedef shared_ptr<std::vector<Block>> PacketArray;

/** \brief provides fragmentation feature at sender
 */
class Slicer : noncopyable
{
public:
  /** \param mtu maximum size of NDNLP header and payload
   *  \note If NDNLP packets are to be encapsulated in an additional header
   *        (eg. in UDP packets), the caller must deduct such overhead.
   */
  explicit
  Slicer(size_t mtu);

  virtual
  ~Slicer();

  PacketArray
  slice(const Block& block);

private:
  template<bool T>
  size_t
  encodeFragment(ndn::EncodingImpl<T>& blk,
                 uint64_t seq, uint16_t fragIndex, uint16_t fragCount,
                 const uint8_t* payload, size_t payloadSize);

  /// estimate the size of NDNLP header and maximum payload size per packet
  void
  estimateOverhead();

private:
  SequenceGenerator m_seqgen;

  /// maximum packet size
  size_t m_mtu;

  /// maximum payload size
  size_t m_maxPayload;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_DAEMON_FACE_NDNLP_SLICER_HPP
