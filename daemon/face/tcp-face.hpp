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

//

inline bool
is_loopback(const boost::asio::ip::address& address)
{
#if BOOST_VERSION < 104700
  if (address.is_v4())
    {
      return (address.to_v4().to_ulong() & 0xFF000000) == 0x7F000000;
    }
  else
    {
      boost::asio::ip::address_v6::bytes_type v6_addr = address.to_v6().to_bytes();
      return ((v6_addr[0] == 0) && (v6_addr[1] == 0)
              && (v6_addr[2] == 0) && (v6_addr[3] == 0)
              && (v6_addr[4] == 0) && (v6_addr[5] == 0)
              && (v6_addr[6] == 0) && (v6_addr[7] == 0)
              && (v6_addr[8] == 0) && (v6_addr[9] == 0)
              && (v6_addr[10] == 0) && (v6_addr[11] == 0)
              && (v6_addr[12] == 0) && (v6_addr[13] == 0)
              && (v6_addr[14] == 0) && (v6_addr[15] == 1));
    }
#else
  return address.is_loopback();
#endif
}

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
    if (!is_loopback(socket.local_endpoint().address()) ||
        !is_loopback(socket.remote_endpoint().address()))
      {
        throw Face::Error("TcpLocalFace can be created only on loopback interface");
      }
  }
};


} // namespace nfd

#endif // NFD_FACE_TCP_FACE_HPP
