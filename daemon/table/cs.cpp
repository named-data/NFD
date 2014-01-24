/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

// XXX This is a fake CS that does not cache anything.

#include "cs.hpp"
namespace ndn {

Cs::Cs()
{
}
  
Cs::~Cs()
{
}

bool
Cs::insert(boost::shared_ptr<Data> data)
{
  return false;
}
  
boost::shared_ptr<cs::Entry>
Cs::find(const Interest& interest)
{
  return boost::shared_ptr<cs::Entry>();
}


};//namespace ndn
