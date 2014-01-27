/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_FORWARDER_HPP
#define NFD_FW_FORWARDER_HPP

#include "common.hpp"
#include "face/face.hpp"

namespace ndn {

/**
 * Forwarder is the main class of NFD.
 * 
 * It creates and owns a set of Face listeners
 */
class Forwarder
{
public:
  Forwarder(boost::asio::io_service& ioService);

  uint64_t
  addFace(const shared_ptr<Face>& face);

  void
  removeFace(const shared_ptr<Face>& face);

  void
  onInterest(const Face& face, const Interest& interest);

  void
  onData(const Face& face, const Data& data);
  
private:
  // container< shared_ptr<Face> > m_faces;
};

}

#endif // NFD_FW_FORWARDER_HPP
