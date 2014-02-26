/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_FACE_HPP
#define NFD_FACE_TCP_FACE_HPP

#include "stream-face.hpp"

namespace nfd
{

/**
 * \brief Implementation of Face abstraction that uses TCP
 *        as underlying transport mechanism
 */
class TcpFace : public StreamFace<boost::asio::ip::tcp>
{
public:
  typedef boost::asio::ip::tcp protocol;

  explicit
  TcpFace(const shared_ptr<protocol::socket>& socket);
};

//

/**
 * \brief Implementation of Face abstraction that uses TCP
 *        as underlying transport mechanism and is used for
 *        local communication (can enable LocalControlHeader)
 */
class TcpLocalFace : public StreamFace<boost::asio::ip::tcp, LocalFace>
{
public:
  typedef boost::asio::ip::tcp protocol;

  explicit
  TcpLocalFace(const shared_ptr<protocol::socket>& socket);
};


/** \brief Class validating use of TcpLocalFace
 */
template<>
struct StreamFaceValidator<boost::asio::ip::tcp, LocalFace>
{
  /** Check that local endpoint is loopback
   *
   *  @throws Face::Error if validation failed
   */
  static void
  validateSocket(boost::asio::ip::tcp::socket& socket)
  {
    if (!socket.local_endpoint().address().is_loopback() ||
        !socket.remote_endpoint().address().is_loopback())
      {
        throw Face::Error("TcpLocalFace can be created only on loopback interface");
      }
  }
};


} // namespace nfd

#endif // NFD_FACE_TCP_FACE_HPP
