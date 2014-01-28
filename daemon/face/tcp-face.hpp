/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_TCP_FACE_HPP
#define NFD_FACE_TCP_FACE_HPP

#include "stream-face.hpp"

namespace ndn
{

/**
 * \brief Implementation of Face abstraction that uses TCP
 *        as underlying transport mechanism
 */
class TcpFace : public StreamFace<boost::asio::ip::tcp>
{
public:
  typedef boost::asio::ip::tcp protocol;

  TcpFace(FaceId id,
          const shared_ptr<protocol::socket>& socket);

  // from Face
  virtual void
  sendInterest(const Interest& interest);

  virtual void
  sendData(const Data& data);
};

} // namespace ndn

#endif // NFD_FACE_TCP_FACE_HPP
