/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-face.hpp"

namespace nfd {

// The whole purpose of this file is to specialize the logger,
// otherwise, everything could be put into the header file.

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(StreamFace, UnixStreamFace::protocol, "UnixStreamFace");

UnixStreamFace::UnixStreamFace(const shared_ptr<UnixStreamFace::protocol::socket>& socket)
  : StreamFace<protocol>(socket)
{
}

bool
UnixStreamFace::isLocal() const
{
  return true;
}

} // namespace nfd
