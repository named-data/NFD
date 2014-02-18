/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_MULTICAST_UDP_FACE_HPP
#define NFD_FACE_MULTICAST_UDP_FACE_HPP

#include "udp-face.hpp"

namespace nfd
{

/**
 * \brief Implementation of Face abstraction that uses 
 *        multicast UDP as underlying transport mechanism
 */
class MulticastUdpFace : public UdpFace
{
public:
  typedef boost::asio::ip::udp protocol;

  /**
   * \brief Creates a Udp face for multicast communication
   */
  explicit
  MulticastUdpFace(const shared_ptr<protocol::socket>& socket);

  const boost::asio::ip::udp::endpoint&
  getMulticastGroup() const;

  // from Face
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);
  
  virtual bool
  isMultiAccess() const;
  
private:
  boost::asio::ip::udp::endpoint m_multicastGroup;
};

} // namespace nfd

#endif // NFD_FACE_MULTICAST_UDP_FACE_HPP
