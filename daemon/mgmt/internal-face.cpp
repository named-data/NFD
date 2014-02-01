/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "internal-face.hpp"

namespace nfd {

NFD_LOG_INIT("InternalFace");

InternalFace::InternalFace()
{

}

void
InternalFace::sendInterest(const Interest& interest)
{
  const Name& interestName(interest.getName());
  NFD_LOG_DEBUG("received Interest: " << interestName);

  size_t nComps = interestName.size();
  for (size_t i = 0; i < nComps; i++)
    {
      Name prefix(interestName.getPrefix(nComps - i));
      std::map<Name, OnInterest>::const_iterator filter =
        m_interestFilters.find(prefix);

      if (filter != m_interestFilters.end())
        {
          NFD_LOG_DEBUG("found Interest filter for " << prefix);
          filter->second(interestName, interest);
        }
      else
        {
          NFD_LOG_DEBUG("no Interest filter found for " << prefix);
        }
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
  NFD_LOG_INFO("registering callback for " << filter);
  m_interestFilters[filter] = onInterest;
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
