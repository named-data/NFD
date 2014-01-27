/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_STREAM_FACE_HPP
#define NFD_FACE_STREAM_FACE_HPP

#include "face.hpp"

namespace ndn {

template <class T>
class StreamFace : public Face
{
public:
  typedef T protocol;

  StreamFace(FaceId id)
    : Face(id)
  {
  }
  
};

} // namespace ndn

#endif // NFD_FACE_STREAM_FACE_HPP
