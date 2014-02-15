/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ndnlp-sequence-generator.hpp"

namespace nfd {
namespace ndnlp {

SequenceBlock::SequenceBlock(uint64_t start, size_t count)
  : m_start(start)
  , m_count(count)
{
}

SequenceGenerator::SequenceGenerator()
  : m_next(0)
{
}

SequenceBlock
SequenceGenerator::nextBlock(size_t count)
{
  SequenceBlock sb(m_next, count);
  m_next += count;
  return sb;
}

} // namespace ndnlp
} // namespace nfd
