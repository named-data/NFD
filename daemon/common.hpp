/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_COMMON_HPP
#define NFD_COMMON_HPP

#include "config.hpp"

#ifdef WITH_TESTS
#define VIRTUAL_WITH_TESTS virtual
#define PUBLIC_WITH_TESTS_ELSE_PROTECTED public
#define PUBLIC_WITH_TESTS_ELSE_PRIVATE public
#define PROTECTED_WITH_TESTS_ELSE_PRIVATE protected
#else
#define VIRTUAL_WITH_TESTS
#define PUBLIC_WITH_TESTS_ELSE_PROTECTED protected
#define PUBLIC_WITH_TESTS_ELSE_PRIVATE private
#define PROTECTED_WITH_TESTS_ELSE_PRIVATE private
#endif

#include <ndn-cpp-dev/interest.hpp>
#include <ndn-cpp-dev/data.hpp>

#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/functional/hash.hpp>

#include <vector>
#include <list>
#include <queue>
#include <set>
#include <sstream>
#include <istream>
#include <fstream>
#include <algorithm>
#include <numeric>

#include "core/logger.hpp"

namespace nfd {

using boost::noncopyable;
using boost::shared_ptr;
using boost::enable_shared_from_this;
using boost::make_shared;
using boost::static_pointer_cast;
using boost::dynamic_pointer_cast;
using boost::weak_ptr;
using boost::function;
using boost::bind;

using ndn::Interest;
using ndn::Data;
using ndn::Name;
using ndn::Exclude;
using ndn::Block;
namespace tlv {
using namespace ndn::Tlv;
}

namespace time = ndn::time;

} // namespace nfd

#endif // NFD_COMMON_HPP
