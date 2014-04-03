/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "udp-face.hpp"

namespace nfd {

NFD_LOG_INCLASS_TEMPLATE_SPECIALIZATION_DEFINE(DatagramFace, UdpFace::protocol, "UdpFace");

UdpFace::UdpFace(const shared_ptr<UdpFace::protocol::socket>& socket, bool isOnDemand)
  : DatagramFace<protocol>(FaceUri(socket->remote_endpoint()),
                           FaceUri(socket->local_endpoint()),
                           socket, isOnDemand)
{
}

void
UdpFace::handleFirstReceive(const uint8_t* buffer,
                            std::size_t nBytesReceived,
                            const boost::system::error_code& error)
{
  NFD_LOG_TRACE("handleFirstReceive");

  // Checking if the received message size is too big.
  // This check is redundant, since in the actual implementation
  // a packet cannot be larger than MAX_NDN_PACKET_SIZE.
  if (!error && (nBytesReceived > MAX_NDN_PACKET_SIZE))
    {
      NFD_LOG_WARN("[id:" << this->getId()
                   << ",endpoint:" << m_socket->local_endpoint()
                   << "] Received message too big. Maximum size is "
                   << MAX_NDN_PACKET_SIZE );
      return;
    }

  receiveDatagram(buffer, nBytesReceived, error);
}

} // namespace nfd
