/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_FACE_COUNTER_HPP
#define NFD_FACE_FACE_COUNTER_HPP

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

#endif // NFD_FACE_FACE_COUNTER_HPP
