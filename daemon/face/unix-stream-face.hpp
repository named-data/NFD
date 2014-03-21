/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UNIX_STREAM_FACE_HPP
#define NFD_FACE_UNIX_STREAM_FACE_HPP

#include "stream-face.hpp"

#ifndef HAVE_UNIX_SOCKETS
#error "Cannot include this file when UNIX sockets are not available"
#endif

namespace nfd
{

/**
 * \brief Implementation of Face abstraction that uses stream-oriented
 *        Unix domain sockets as underlying transport mechanism
 */
class UnixStreamFace : public StreamFace<boost::asio::local::stream_protocol, LocalFace>
{
public:
  typedef boost::asio::local::stream_protocol protocol;

  explicit
  UnixStreamFace(const shared_ptr<protocol::socket>& socket);
};

} // namespace nfd

#endif // NFD_FACE_UNIX_STREAM_FACE_HPP
