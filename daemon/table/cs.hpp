/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_CS_HPP
#define NFD_TABLE_CS_HPP

#include "common.hpp"
#include "cs-entry.hpp"

namespace nfd {

/** \class Cs
 *  \brief represents the ContentStore
 */
class Cs : noncopyable
{
public:
  Cs();
  
  ~Cs();
  
  /** \brief inserts a Data packet
   *  The caller should ensure that this Data packet is admittable,
   *  ie. not unsolicited from a untrusted source.
   *  This does not guarantee the Data is added to the cache;
   *  even if the Data is added, it may be removed anytime in the future.
   *  \return{ whether the Data is added }
   */
  bool
  insert(const Data& data);
  
  /** \brief finds the best match Data for an Interest
   *  \return{ the best match, if any; otherwise 0 }
   */
  const Data*
  find(const Interest& interest);
};

} // namespace nfd

#endif // NFD_TABLE_CS_HPP
