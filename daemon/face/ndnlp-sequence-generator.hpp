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

#ifndef NFD_DAEMON_FACE_NDNLP_SEQUENCE_GENERATOR_HPP
#define NFD_DAEMON_FACE_NDNLP_SEQUENCE_GENERATOR_HPP

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
    BOOST_THROW_EXCEPTION(std::out_of_range("pos"));
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

#endif // NFD_DAEMON_FACE_NDNLP_SEQUENCE_GENERATOR_HPP
