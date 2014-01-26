/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_COMMON_HPP
#define NFD_COMMON_HPP

#include "config.hpp"

#include <ndn-cpp-dev/interest.hpp>
#include <ndn-cpp-dev/data.hpp>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>

#include <vector>
#include <list>
#include <set>

namespace ndn {

using boost::noncopyable;
using boost::shared_ptr;
using boost::make_shared;
using boost::function;
using boost::bind;

} // namespace ndn

#endif // NFD_COMMON_HPP
