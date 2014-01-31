/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "internal-face.hpp"

namespace nfd {

InternalFace::InternalFace()
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
