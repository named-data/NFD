/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-face.hpp"

namespace nfd {

// The whole purpose of this file is to specialize the logger,
// otherwise, everything could be put into the header file.

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(StreamFace, TcpFace::protocol, "TcpFace");

TcpFace::TcpFace(const shared_ptr<TcpFace::protocol::socket>& socket)
  : StreamFace<protocol>(socket)
{
}

bool
TcpFace::isLocal() const
{
  return false;
}


} // namespace nfd
