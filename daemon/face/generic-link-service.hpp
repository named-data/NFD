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

#ifndef NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
#define NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP

#include "common.hpp"
#include "core/logger.hpp"

#include "link-service.hpp"

#include <ndn-cxx/lp/packet.hpp>

namespace nfd {
namespace face {

/** \brief GenericLinkService is a LinkService that implements the NDNLPv2 protocol
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
 */
class GenericLinkService : public LinkService
{
public:
  /** \brief Options that control the behavior of GenericLinkService
   */
  class Options
  {
  public:
    Options();

  public:
    // TODO #3171: fragmentation and reassembly options

    /** \brief enables encoding of IncomingFaceId, and decoding of NextHopFaceId and CachePolicy
     */
    bool allowLocalFields;
  };

  explicit
  GenericLinkService(const Options& options = Options());

  /** \brief get Options used by GenericLinkService
   */
  const Options&
  getOptions() const;

  /** \brief sets Options used by GenericLinkService
   */
  void
  setOptions(const Options& options);

private: // send path entrypoint
  /** \brief sends Interest
   */
  void
  doSendInterest(const Interest& interest) DECL_OVERRIDE;

  /** \brief sends Data
   */
  void
  doSendData(const Data& data) DECL_OVERRIDE;

  /** \brief sends Nack
   *  This class does not send out a Nack.
   */
  void
  doSendNack(const ndn::lp::Nack& nack) DECL_OVERRIDE;

private: // receive path entrypoint
  /** \brief receives Packet
   */
  void
  doReceivePacket(Transport::Packet&& packet) DECL_OVERRIDE;

private: // encoding and decoding
  /** \brief encode IncomingFaceId into LpPacket and verify local fields
   */
  static bool
  encodeLocalFields(const Interest& interest, lp::Packet& lpPacket);

  /** \brief encode CachingPolicy and IncomingFaceId into LpPacket and verify local fields
   */
  static bool
  encodeLocalFields(const Data& data, lp::Packet& lpPacket);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must not have Nack field
   *
   *  If decoding is successful, receiveInterest signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeInterest(const Block& netPkt, const lp::Packet& firstPkt);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Data
   *  \param firstPkt LpPacket of first fragment
   *
   *  If decoding is successful, receiveData signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeData(const Block& netPkt, const lp::Packet& firstPkt);

  /** \brief decode incoming Interest
   *  \param netPkt reassembled network-layer packet; TLV-TYPE must be Interest
   *  \param firstPkt LpPacket of first fragment; must have Nack field
   *
   *  If decoding is successful, receiveNack signal is emitted;
   *  otherwise, a warning is logged.
   *
   *  \throw tlv::Error parse error in an LpHeader field
   */
  void
  decodeNack(const Block& netPkt, const lp::Packet& firstPkt);

private:
  Options m_options;
};

inline const GenericLinkService::Options&
GenericLinkService::getOptions() const
{
  return m_options;
}

inline void
GenericLinkService::setOptions(const GenericLinkService::Options& options)
{
  m_options = options;
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_GENERIC_LINK_SERVICE_HPP
