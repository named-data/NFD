/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_NDNLP_SEQUENCE_GENERATOR_HPP
#define NFD_FACE_NDNLP_SEQUENCE_GENERATOR_HPP

#include "common.hpp"

namespace nfd {
namespace ndnlp {

/** \brief represents a block of sequence numbers
 */
class SequenceBlock
{
public:
  SequenceBlock(uint64_t start, size_t count);
  
  /** \return{ the pos-th sequence number }
   */
  uint64_t
  operator[](size_t pos) const;
  
  size_t
  count() const;
  
private:
  uint64_t m_start;
  size_t m_count;
};

inline uint64_t
SequenceBlock::operator[](size_t pos) const
{
  if (pos >= m_count)
    throw std::out_of_range("pos");
  return m_start + static_cast<uint64_t>(pos);
}

inline size_t
SequenceBlock::count() const
{
  return m_count;
}

/** \class SequenceGenerator
 *  \brief generates sequence numbers
 */
class SequenceGenerator : noncopyable
{
public:
  SequenceGenerator();
  
  /** \brief generates a block of consecutive sequence numbers
   *
   *  This block must not overlap with a recent block.
   */
  SequenceBlock
  nextBlock(size_t count);

private:
  /// next sequence number
  uint64_t m_next;
};

} // namespace ndnlp
} // namespace nfd

#endif // NFD_FACE_NDNLP_SEQUENCE_GENERATOR_HPP
