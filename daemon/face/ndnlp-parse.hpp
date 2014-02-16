/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_NDNLP_PARSE_HPP
#define NFD_FACE_NDNLP_PARSE_HPP

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

#endif // NFD_FACE_NDNLP_PARSE_HPP
