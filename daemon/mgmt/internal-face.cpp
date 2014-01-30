/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "internal-face.hpp"
#include "fib-manager.hpp"

namespace nfd {

InternalFace::InternalFace(FibManager& manager)
  : m_fibManager(manager)
{

}

void
InternalFace::sendInterest(const Interest& interest)
{
  const Name& interestName = interest.getName();

  if (m_fibManager.getRequestPrefix().isPrefixOf(interestName))
    {
      m_fibManager.onFibRequest(interest);
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
