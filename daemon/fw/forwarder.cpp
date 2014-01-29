/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "forwarder.hpp"

namespace nfd {

Forwarder::Forwarder(boost::asio::io_service& ioService)
{
}

uint64_t
Forwarder::addFace(const shared_ptr<Face>& face)
{
  return -1;
}

void
Forwarder::removeFace(const shared_ptr<Face>& face)
{
}

void
Forwarder::onInterest(const Face& face, const Interest& interest)
{
}

void
Forwarder::onData(const Face& face, const Data& data)
{
}

} // namespace nfd
