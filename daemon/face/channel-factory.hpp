/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_CHANNEL_FACTORY_HPP
#define NFD_FACE_CHANNEL_FACTORY_HPP

#include "common.hpp"

namespace ndn {

/**
 * \brief Base class for all channel factories
 */
template<class E, class C>
class ChannelFactory
{
public:
  typedef E Endpoint;
  typedef C Channel;
  
  /**
   * \brief Base class for all exceptions thrown by channel factories
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

protected:
  typedef std::map<Endpoint, Channel> ChannelMap;
  ChannelMap m_channels;  
};

} // namespace ndn

#endif // NFD_FACE_CHANNEL_FACTORY_HPP
