/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FACE_PROTOCOL_FACTORY_HPP
#define NFD_FACE_PROTOCOL_FACTORY_HPP

#include "common.hpp"

namespace nfd {

class ProtocolFactory
{
public:
  /**
   * \brief Base class for all exceptions thrown by channel factories
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };
};

} // namespace nfd

#endif // NFD_FACE_PROTOCOL_FACTORY_HPP
