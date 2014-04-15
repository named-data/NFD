/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_DAEMON_FACE_NDNLP_PARSE_HPP
#define NFD_DAEMON_FACE_NDNLP_PARSE_HPP

#include "common.hpp"
#include "ndnlp-tlv.hpp"

namespace nfd {
namespace ndnlp {

struct ParseError : public std::runtime_error
{
  ParseError(const std::string& what)
    : std::runtime_error(what)
  {
  }
};

/** \brief represents a NdnlpData packet
 *
 *  NdnlpData ::= NDNLP-DATA-TYPE TLV-LENGTH
 *                  NdnlpSequence
 *                  NdnlpFragIndex?
 *                  NdnlpFragCount?
 *                  NdnlpPayload
 */
class NdnlpData
{
public:
  /** \brief parse a NdnlpData packet
   *
   *  \exception ParseError packet is malformated
   */
  void
  wireDecode(const Block& wire);

public:
  uint64_t m_seq;
  uint16_t m_fragIndex;
  uint16_t m_fragCount;
  Block m_payload;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_DAEMON_FACE_NDNLP_PARSE_HPP
