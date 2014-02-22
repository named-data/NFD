/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face-counter.hpp"

namespace nfd {

FaceCounters::FaceCounters()
  : m_inInterest(0)
  , m_inData(0)
  , m_outInterest(0)
  , m_outData(0)
{
}

} //namespace nfd
