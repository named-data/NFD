/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NRD_COMMON_HPP
#define NRD_COMMON_HPP

#include <iostream>
#include <list>

#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/security/validator-config.hpp>
#include <ndn-cpp-dev/util/scheduler.hpp>

#include <ndn-cpp-dev/management/nfd-controller.hpp>
#include <ndn-cpp-dev/management/nfd-control-response.hpp>
#include <ndn-cpp-dev/management/nrd-prefix-reg-options.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "../build/src/config.hpp"

namespace ndn {
namespace nrd {

class Error : public std::runtime_error
{
public:
  explicit
  Error(const std::string& what)
    : std::runtime_error(what)
  {
  }
};

} //namespace nrd
} //namespace ndn
#endif // NRD_COMMON_HPP
