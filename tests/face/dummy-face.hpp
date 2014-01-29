/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_FACE_DUMMY_FACE_HPP
#define NFD_TEST_FACE_DUMMY_FACE_HPP

#include "face/face.hpp"

namespace nfd {

/** \class DummyFace
 *  \brief provides a Face that cannot communicate
 *  for unit testing only
 */
class DummyFace : public Face
{
public:
  DummyFace(FaceId id)
    : Face(id)
  {
  }
  
  virtual void
  sendInterest(const Interest &interest)
  {
  }
  
  virtual void
  sendData(const Data &data)
  {
  }
};

} // namespace nfd

#endif // TEST_FACE_DUMMY_FACE_HPP
