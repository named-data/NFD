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
 */
typedef uint64_t FaceCounter;


/** \brief contains counters on face
 */
class FaceCounters
{
public:
  FaceCounters();

  /// incoming Interest (total packets since Face establishment)
  const FaceCounter&
  getInInterest() const;

  FaceCounter&
  getInInterest();

  /// incoming Data (total packets since Face establishment)
  const FaceCounter&
  getInData() const;

  FaceCounter&
  getInData();

  /// outgoing Interest (total packets since Face establishment)
  const FaceCounter&
  getOutInterest() const;

  FaceCounter&
  getOutInterest();

  /// outgoing Data (total packets since Face establishment)
  const FaceCounter&
  getOutData() const;

  FaceCounter&
  getOutData();

private:
  FaceCounter m_inInterest;
  FaceCounter m_inData;
  FaceCounter m_outInterest;
  FaceCounter m_outData;
};


inline const FaceCounter&
FaceCounters::getInInterest() const
{
  return m_inInterest;
}

inline FaceCounter&
FaceCounters::getInInterest()
{
  return m_inInterest;
}

inline const FaceCounter&
FaceCounters::getInData() const
{
  return m_inData;
}

inline FaceCounter&
FaceCounters::getInData()
{
  return m_inData;
}

inline const FaceCounter&
FaceCounters::getOutInterest() const
{
  return m_outInterest;
}

inline FaceCounter&
FaceCounters::getOutInterest()
{
  return m_outInterest;
}

inline const FaceCounter&
FaceCounters::getOutData() const
{
  return m_outData;
}

inline FaceCounter&
FaceCounters::getOutData()
{
  return m_outData;
}


} // namespace nfd

#endif // NFD_FACE_FACE_COUNTER_HPP
