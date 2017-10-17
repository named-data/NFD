/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NFD_TESTS_DAEMON_FACE_TRANSPORT_TEST_COMMON_HPP
#define NFD_TESTS_DAEMON_FACE_TRANSPORT_TEST_COMMON_HPP

#include "face/transport.hpp"
#include "tests/test-common.hpp"
#include "test-ip.hpp"

namespace nfd {
namespace face {
namespace tests {

/** \brief Check that a transport has all its static properties set after initialization
 *
 *  This check shall be inserted to the StaticProperties test case for each transport,
 *  in addition to checking the values of properties.
 *  When a new static property is defined, this test case shall be updated.
 *  Thus, if a transport forgets to set a static property, this check would fail.
 */
inline void
checkStaticPropertiesInitialized(const Transport& transport)
{
  BOOST_CHECK(!transport.getLocalUri().getScheme().empty());
  BOOST_CHECK(!transport.getRemoteUri().getScheme().empty());
  BOOST_CHECK_NE(transport.getScope(), ndn::nfd::FACE_SCOPE_NONE);
  BOOST_CHECK_NE(transport.getPersistency(), ndn::nfd::FACE_PERSISTENCY_NONE);
  BOOST_CHECK_NE(transport.getLinkType(), ndn::nfd::LINK_TYPE_NONE);
  BOOST_CHECK_NE(transport.getMtu(), MTU_INVALID);
}

/** \brief Generic wrapper for transport fixtures that require an IP address
 */
template<typename TransportFixtureBase,
         AddressFamily AF = AddressFamily::Any,
         AddressScope AS = AddressScope::Any,
         MulticastInterface MC = MulticastInterface::Any>
class IpTransportFixture : public TransportFixtureBase
{
protected:
  IpTransportFixture()
  {
    BOOST_TEST_MESSAGE("Testing with AddressFamily=" << AF <<
                       " AddressScope=" << AS <<
                       " MulticastInterface=" << MC <<
                       " TestIp=" << address);
  }

  std::pair<bool, std::string>
  checkPreconditions() const
  {
    return {!address.is_unspecified(), "no appropriate IP address available"};
  }

  template<typename... Args>
  void
  initialize(Args&&... args)
  {
    TransportFixtureBase::initialize(address, std::forward<Args>(args)...);
  }

protected:
  static constexpr AddressFamily addressFamily = AF;
  static constexpr AddressScope addressScope = AS;
  const boost::asio::ip::address address = getTestIp(AF, AS, MC);
};

} // namespace tests
} // namespace face
} // namespace nfd

#define GENERATE_IP_TRANSPORT_FIXTURE_INSTANTIATIONS(F) \
  IpTransportFixture<F, AddressFamily::V4, AddressScope::Loopback>,  \
  IpTransportFixture<F, AddressFamily::V4, AddressScope::Global>,    \
  IpTransportFixture<F, AddressFamily::V6, AddressScope::Loopback>,  \
  IpTransportFixture<F, AddressFamily::V6, AddressScope::LinkLocal>, \
  IpTransportFixture<F, AddressFamily::V6, AddressScope::Global>

#define TRANSPORT_TEST_CHECK_PRECONDITIONS() \
  do { \
    auto result = this->checkPreconditions(); \
    if (!result.first) { \
      BOOST_WARN_MESSAGE(false, "skipping test case: " << result.second); \
      return; \
    } \
  } while (false)

#define TRANSPORT_TEST_INIT(...) \
  do { \
    TRANSPORT_TEST_CHECK_PRECONDITIONS(); \
    this->initialize(__VA_ARGS__); \
  } while (false)

#endif // NFD_TESTS_DAEMON_FACE_TRANSPORT_TEST_COMMON_HPP
