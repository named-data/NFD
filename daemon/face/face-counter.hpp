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

#ifndef NFD_DAEMON_FACE_FACE_COUNTER_HPP
#define NFD_DAEMON_FACE_FACE_COUNTER_HPP

#include "common.hpp"

namespace nfd {

/** \class FaceCounter
 *  \brief represents a counter on face
 *
 *  \todo This class should be noncopyable
 */
typedef uint64_t FaceCounter;


/** \brief contains counters on face
 */
class FaceCounters : noncopyable
{
public:
  FaceCounters();

  /// incoming Interest (total packets since Face establishment)
  const FaceCounter&
  getNInInterests() const;

  FaceCounter&
  getNInInterests();

  /// incoming Data (total packets since Face establishment)
  const FaceCounter&
  getNInDatas() const;

  FaceCounter&
  getNInDatas();

  /// outgoing Interest (total packets since Face establishment)
  const FaceCounter&
  getNOutInterests() const;

  FaceCounter&
  getNOutInterests();

  /// outgoing Data (total packets since Face establishment)
  const FaceCounter&
  getNOutDatas() const;

  FaceCounter&
  getNOutDatas();

private:
  FaceCounter m_nInInterests;
  FaceCounter m_nInDatas;
  FaceCounter m_outInterests;
  FaceCounter m_outDatas;
};

inline
FaceCounters::FaceCounters()
  : m_nInInterests(0)
  , m_nInDatas(0)
  , m_outInterests(0)
  , m_outDatas(0)
{
}

inline const FaceCounter&
FaceCounters::getNInInterests() const
{
  return m_nInInterests;
}

inline FaceCounter&
FaceCounters::getNInInterests()
{
  return m_nInInterests;
}

inline const FaceCounter&
FaceCounters::getNInDatas() const
{
  return m_nInDatas;
}

inline FaceCounter&
FaceCounters::getNInDatas()
{
  return m_nInDatas;
}

inline const FaceCounter&
FaceCounters::getNOutInterests() const
{
  return m_outInterests;
}

inline FaceCounter&
FaceCounters::getNOutInterests()
{
  return m_outInterests;
}

inline const FaceCounter&
FaceCounters::getNOutDatas() const
{
  return m_outDatas;
}

inline FaceCounter&
FaceCounters::getNOutDatas()
{
  return m_outDatas;
}


} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_COUNTER_HPP
