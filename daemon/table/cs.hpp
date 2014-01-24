/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_CS_H
#define NFD_TABLE_CS_H
#include <boost/shared_ptr.hpp>
#include <ndn-cpp-dev/interest.hpp>
#include <ndn-cpp-dev/data.hpp>
#include "cs-entry.hpp"
namespace ndn {

/** \class Cs
 *  \brief represents the ContentStore
 */
class Cs : boost::noncopyable
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
  insert(boost::shared_ptr<Data> data);
  
  /** \brief finds the best match Data for an Interest
   *  \return{ the best match, if any; otherwise null }
   */
  boost::shared_ptr<cs::Entry>
  find(const Interest& interest);
};

};//namespace ndn
#endif//NFD_TABLE_CS_H
