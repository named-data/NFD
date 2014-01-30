/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "internal-face.hpp"

namespace nfd {

const FaceId INTERNAL_FACE_FACEID = 0;

InternalFace::InternalFace()
  : Face(INTERNAL_FACE_FACEID)
{

}

void
InternalFace::sendInterest(const Interest& interest)
{
  static const Name prefixRegPrefix("/localhost/nfd/prefixreg");
  const Name& interestName = interest.getName();

  if (prefixRegPrefix.isPrefixOf(interestName))
    {
      //invoke FibManager
    }
  //Drop Interest
}

void
InternalFace::sendData(const Data& data)
{

}

void
InternalFace::setInterestFilter(const Name& filter,
                                OnInterest onInterest)
{

}

void
InternalFace::put(const Data& data)
{
  onReceiveData(data);
}

InternalFace::~InternalFace()
{

}

} // namespace nfd
