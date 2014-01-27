/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "tcp-face.hpp"

namespace ndn {

TcpFace::TcpFace(FaceId id, const shared_ptr<TcpFace::protocol::socket>& socket)
  : StreamFace<protocol>(id)
  , m_socket(socket)
{
}

void
TcpFace::sendInterest(const Interest& interest)
{
}

void
TcpFace::sendData(const Data& data)
{
}

} // namespace ndn
