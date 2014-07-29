/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#include "common.hpp"

namespace nfd {

/** \brief represents a counter of number of packets
 */
// PacketCounter is noncopyable, because increment should be called on the counter,
// not a copy of it; it's implicitly convertible to uint64_t to be observed
class PacketCounter : noncopyable
{
public:
  typedef uint64_t rep;

  PacketCounter()
    : m_value(0)
  {
  }

  operator rep() const
  {
    return m_value;
  }

  PacketCounter&
  operator++()
  {
    ++m_value;
    return *this;
  }
  // postfix ++ operator is not provided because it's not needed

  void
  set(rep value)
  {
    m_value = value;
  }

private:
  rep m_value;
};

/** \brief represents a counter of number of bytes
 */
// ByteCounter is noncopyable, because increment should be called on the counter,
// not a copy of it; it's implicitly convertible to uint64_t to be observed
class ByteCounter : noncopyable
{
public:
  typedef uint64_t rep;

  ByteCounter()
    : m_value(0)
  {
  }

  operator rep() const
  {
    return m_value;
  }

  ByteCounter&
  operator+=(rep n)
  {
    m_value += n;
    return *this;
  }

  void
  set(rep value)
  {
    m_value = value;
  }

private:
  rep m_value;
};

/** \brief contains network layer packet counters
 */
class NetworkLayerCounters : noncopyable
{
public:
  /// incoming Interest
  const PacketCounter&
  getNInInterests() const
  {
    return m_nInInterests;
  }

  PacketCounter&
  getNInInterests()
  {
    return m_nInInterests;
  }

  /// incoming Data
  const PacketCounter&
  getNInDatas() const
  {
    return m_nInDatas;
  }

  PacketCounter&
  getNInDatas()
  {
    return m_nInDatas;
  }

  /// outgoing Interest
  const PacketCounter&
  getNOutInterests() const
  {
    return m_nOutInterests;
  }

  PacketCounter&
  getNOutInterests()
  {
    return m_nOutInterests;
  }

  /// outgoing Data
  const PacketCounter&
  getNOutDatas() const
  {
    return m_nOutDatas;
  }

  PacketCounter&
  getNOutDatas()
  {
    return m_nOutDatas;
  }

protected:
  /** \brief copy current obseverations to a struct
   *  \param recipient an object with set methods for counters
   */
  template<typename R>
  void
  copyTo(R& recipient) const
  {
    recipient.setNInInterests(this->getNInInterests());
    recipient.setNInDatas(this->getNInDatas());
    recipient.setNOutInterests(this->getNOutInterests());
    recipient.setNOutDatas(this->getNOutDatas());
  }

private:
  PacketCounter m_nInInterests;
  PacketCounter m_nInDatas;
  PacketCounter m_nOutInterests;
  PacketCounter m_nOutDatas;
};

/** \brief contains link layer byte counters
 */
class LinkLayerCounters : noncopyable
{
public:
  /// received bytes
  const ByteCounter&
  getNInBytes() const
  {
    return m_nInBytes;
  }

  ByteCounter&
  getNInBytes()
  {
    return m_nInBytes;
  }

  /// sent bytes
  const ByteCounter&
  getNOutBytes() const
  {
    return m_nOutBytes;
  }

  ByteCounter&
  getNOutBytes()
  {
    return m_nOutBytes;
  }

protected:
  /** \brief copy current obseverations to a struct
   *  \param recipient an object with set methods for counters
   */
  template<typename R>
  void
  copyTo(R& recipient) const
  {
    recipient.setNInBytes(this->getNInBytes());
    recipient.setNOutBytes(this->getNOutBytes());
  }

private:
  ByteCounter m_nInBytes;
  ByteCounter m_nOutBytes;
};

/** \brief contains counters on face
 */
class FaceCounters : public NetworkLayerCounters, public LinkLayerCounters
{
public:
  /** \brief copy current obseverations to a struct
   *  \param recipient an object with set methods for counters
   */
  template<typename R>
  void
  copyTo(R& recipient) const
  {
    this->NetworkLayerCounters::copyTo(recipient);
    this->LinkLayerCounters::copyTo(recipient);
  }
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_COUNTERS_HPP
