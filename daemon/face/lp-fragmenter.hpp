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

#ifndef NFD_DAEMON_FACE_LP_FRAGMENTER_HPP
#define NFD_DAEMON_FACE_LP_FRAGMENTER_HPP

#include "face-log.hpp"

#include <ndn-cxx/lp/packet.hpp>

namespace nfd {
namespace face {

class LinkService;

/** \brief fragments network-layer packets into NDNLPv2 link-layer packets
 *  \sa http://redmine.named-data.net/projects/nfd/wiki/NDNLPv2
 */
class LpFragmenter
{
public:
  /** \brief Options that control the behavior of LpFragmenter
   */
  class Options
  {
  public:
    Options();

  public:
    /** \brief maximum number of fragments in a packet
     */
    size_t nMaxFragments;
  };

  explicit
  LpFragmenter(const Options& options = Options(), const LinkService* linkService = nullptr);

  /** \brief set options for fragmenter
   */
  void
  setOptions(const Options& options);

  /** \return LinkService that owns this instance
   *
   *  This is only used for logging, and may be nullptr.
   */
  const LinkService*
  getLinkService() const;

  /** \brief fragments a network-layer packet into link-layer packets
   *  \param packet an LpPacket that contains a network-layer packet;
   *                must have Fragment field, must not have FragIndex and FragCount fields
   *  \param mtu maximum allowable LpPacket size after fragmentation and sequence number assignment
   *  \return whether fragmentation succeeded, fragmented packets without sequence number
   */
  std::tuple<bool, std::vector<lp::Packet>>
  fragmentPacket(const lp::Packet& packet, size_t mtu);

private:
  Options m_options;
  const LinkService* m_linkService;
};

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<LpFragmenter>& flh);

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_LP_FRAGMENTER_HPP
