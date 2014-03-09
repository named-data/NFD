/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_UDP_FACE_HPP
#define NFD_FACE_UDP_FACE_HPP

#include "datagram-face.hpp"

namespace nfd
{

/**
 * \brief Implementation of Face abstraction that uses UDP
 *        as underlying transport mechanism
 */
class UdpFace : public DatagramFace<boost::asio::ip::udp>
{
public:
  typedef boost::asio::ip::udp protocol;

  UdpFace(const shared_ptr<protocol::socket>& socket,
          bool isPermanent);

  //@todo if needed by other datagramFaces, it could be moved to datagram-face.hpp
  /**
   * \brief Manages the first datagram received by the UdpChannel socket set on listening
   */
  void
  handleFirstReceive(const uint8_t* buffer,
                     std::size_t nBytesReceived,
                     const boost::system::error_code& error);

};

} // namespace nfd

#endif // NFD_FACE_UDP_FACE_HPP
