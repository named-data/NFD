/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "internal-face.hpp"
#include "core/logger.hpp"
#include "core/global-io.hpp"

namespace nfd {

NFD_LOG_INIT("InternalFace");

InternalFace::InternalFace()
  : Face(FaceUri("internal://"), FaceUri("internal://"), true)
{
}

void
InternalFace::sendInterest(const Interest& interest)
{
  onSendInterest(interest);

  // Invoke .processInterest a bit later,
  // to avoid potential problems in forwarding pipelines.
  getGlobalIoService().post(bind(&InternalFace::processInterest,
                                 this, interest.shared_from_this()));
}

void
InternalFace::processInterest(const shared_ptr<const Interest>& interest)
{
  if (m_interestFilters.size() == 0)
    {
      NFD_LOG_DEBUG("no Interest filters to match against");
      return;
    }

  const Name& interestName(interest->getName());
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
          filter->second(interestName, *interest);
        }
      else
        {
          NFD_LOG_DEBUG("no Interest filter found for " << interestName << " (before end)");
        }
    }
  else if (filter->first == interestName)
    {
      NFD_LOG_DEBUG("found Interest filter for " << filter->first << " (exact match)");
      filter->second(interestName, *interest);
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
          filter->second(interestName, *interest);
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
  onSendData(data);
}

void
InternalFace::close()
{
  throw Error("Internal face cannot be closed");
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
