/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "rib.hpp"

namespace ndn {
namespace nrd {

Rib::Rib()
{
}


Rib::~Rib()
{
}

static inline bool
compareNameFaceProtocol(const PrefixRegOptions& opt1, const PrefixRegOptions& opt2)
{
  return (opt1.getName() == opt2.getName() &&
          opt1.getFaceId() == opt2.getFaceId() &&
          opt1.getProtocol() == opt2.getProtocol());
}


Rib::const_iterator
Rib::find(const PrefixRegOptions& options) const
{
  RibTable::const_iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                             bind(&compareNameFaceProtocol, _1, options));
  if (it == m_rib.end())
    {
      return end();
    }
  else
    return it;
}


void
Rib::insert(const PrefixRegOptions& options)
{
  RibTable::iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                       bind(&compareNameFaceProtocol, _1, options));
  if (it == m_rib.end())
    {
      m_rib.push_front(options);
    }
  else
    {
      //entry exist, update other fields
      it->setFlags(options.getFlags());
      it->setCost(options.getCost());
      it->setExpirationPeriod(options.getExpirationPeriod());
      it->setProtocol(options.getProtocol());
    }
}


void
Rib::erase(const PrefixRegOptions& options)
{
  RibTable::iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                       bind(&compareNameFaceProtocol, _1, options));
  if (it != m_rib.end())
    {
      m_rib.erase(it);
    }
}

void
Rib::erase(uint64_t faceId)
{
  //Keep it simple for now, with Trie this will be changed.
  RibTable::iterator it = m_rib.begin();
  while (it != m_rib.end())
  {
    if (it->getFaceId() == faceId)
      it = m_rib.erase(it);
    else
      ++it;
  }
}

} // namespace nrd
} // namespace ndn
