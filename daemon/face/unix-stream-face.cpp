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

BOOST_STATIC_ASSERT((boost::is_same<UnixStreamFace::protocol::socket::native_handle_type,
                     int>::value));

UnixStreamFace::UnixStreamFace(const shared_ptr<UnixStreamFace::protocol::socket>& socket)
  : StreamFace<protocol, LocalFace>(FaceUri::fromFd(socket->native_handle()),
                                    FaceUri(socket->local_endpoint()),
                                    socket, true)
{
}

} // namespace nfd
