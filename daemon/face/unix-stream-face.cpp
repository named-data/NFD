/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "unix-stream-face.hpp"

namespace nfd {

// The whole purpose of this file is to specialize the logger,
// otherwise, everything could be put into the header file.

NFD_LOG_INCLASS_2TEMPLATE_SPECIALIZATION_DEFINE(StreamFace,
                                                UnixStreamFace::protocol, LocalFace,
                                                "UnixStreamFace");

UnixStreamFace::UnixStreamFace(const shared_ptr<UnixStreamFace::protocol::socket>& socket)
  : StreamFace<protocol, LocalFace>(socket)
{
}

} // namespace nfd
