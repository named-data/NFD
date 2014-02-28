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

//
Rib::~Rib()
{
}

static inline bool
ribEntryNameCompare(const PrefixRegOptions& opt1, const PrefixRegOptions& opt2)
{
  return opt1.getName() == opt2.getName();
}

//
Rib::const_iterator
Rib::find(const PrefixRegOptions& options) const
{
  RibTable::const_iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                             bind(&ribEntryNameCompare, _1, options));
  if (it == m_rib.end())
    {
      return end();
    }
  else
    return it;
}

//
void
Rib::insert(const PrefixRegOptions& options)
{
  RibTable::const_iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                             bind(&ribEntryNameCompare, _1, options));
  if (it == m_rib.end())
    {
      m_rib.push_front(options);
    }
}

//
void
Rib::erase(const PrefixRegOptions& options)
{
  RibTable::iterator it = std::find_if(m_rib.begin(), m_rib.end(),
                                       bind(&ribEntryNameCompare, _1, options));
  if (it != m_rib.end())
    {
      m_rib.erase(it);
    }
}

} // namespace nrd
} // namespace ndn
