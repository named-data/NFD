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
  if (m_interestFilters.size() == 0)
    {
      NFD_LOG_DEBUG("no Interest filters to match against");
      return;
    }

  const Name& interestName(interest.getName());
  NFD_LOG_DEBUG("received Interest: " << interestName);

  std::map<Name, OnInterest>::const_iterator filter =
    m_interestFilters.lower_bound(interestName);

  // lower_bound gives us the first Name that is
  // an exact match OR ordered after interestName.
  //
  // If we reach the end of the map, then we need
  // only check if the before-end element is a match.
  //
  // If we match an element, then the current
  // position or the previous element are potential
  // matches.
  //
  // If we hit begin, the element is either an exact
  // match or there is no matching prefix in the map.


  if (filter == m_interestFilters.end() && filter != m_interestFilters.begin())
    {
      // We hit the end, check if the previous element
      // is a match
      --filter;
      if (filter->first.isPrefixOf(interestName))
        {
          NFD_LOG_DEBUG("found Interest filter for " << filter->first << " (before end match)");
          filter->second(interestName, interest);
        }
      else
        {
          NFD_LOG_DEBUG("no Interest filter found for " << interestName << " (before end)");
        }
    }
  else if (filter->first == interestName)
    {
      NFD_LOG_DEBUG("found Interest filter for " << filter->first << " (exact match)");
      filter->second(interestName, interest);
    }
  else if (filter != m_interestFilters.begin())
    {
      // the element we found is canonically
      // ordered after interestName.
      // Check the previous element.
      --filter;
      if (filter->first.isPrefixOf(interestName))
        {
          NFD_LOG_DEBUG("found Interest filter for " << filter->first << " (previous match)");
          filter->second(interestName, interest);
        }
      else
        {
          NFD_LOG_DEBUG("no Interest filter found for " << interestName << " (previous)");
        }
    }
  else
    {
      NFD_LOG_DEBUG("no Interest filter found for " << interestName << " (begin)");
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
