/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "channel.hpp"

namespace nfd {

Channel::~Channel()
{
}

void
Channel::setUri(const FaceUri& uri)
{
  m_uri = uri;
}

} // namespace nfd
