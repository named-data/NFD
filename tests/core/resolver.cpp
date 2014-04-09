/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "core/resolver.hpp"
#include "core/logger.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("tests.CoreResolver");

using boost::asio::ip::address_v4;
using boost::asio::ip::address_v6;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

BOOST_FIXTURE_TEST_SUITE(CoreResolver, BaseFixture)

template<class Protocol>
class ResolverFixture : protected BaseFixture
{
public:
  ResolverFixture()
    : m_nFailures(0)
    , m_nSuccesses(0)
  {
  }

  void
  onSuccess(const typename Protocol::endpoint& resolvedEndpoint,
            const typename Protocol::endpoint& expectedEndpoint,
            bool isValid, bool wantCheckAddress = false)
  {
    NFD_LOG_DEBUG("Resolved to: " << resolvedEndpoint);
    ++m_nSuccesses;

    if (!isValid)
      {
        BOOST_FAIL("Resolved to " + boost::lexical_cast<std::string>(resolvedEndpoint)
                   + ", but it should have failed");
      }

    BOOST_CHECK_EQUAL(resolvedEndpoint.port(), expectedEndpoint.port());

    BOOST_CHECK_EQUAL(resolvedEndpoint.address().is_v4(), expectedEndpoint.address().is_v4());

    // checking address is not deterministic and should be enabled only
    // if only one IP address will be returned by resolution
    if (wantCheckAddress)
      {
        BOOST_CHECK_EQUAL(resolvedEndpoint.address(), expectedEndpoint.address());
      }
  }

  void
  onFailure(bool isValid)
  {
    ++m_nFailures;

    if (!isValid)
      BOOST_FAIL("Resolution should not have failed");

    BOOST_CHECK_MESSAGE(true, "Resolution failed as expected");
  }


  uint32_t m_nFailures;
  uint32_t m_nSuccesses;
};

BOOST_FIXTURE_TEST_CASE(Tcp, ResolverFixture<tcp>)
{
  TcpResolver::asyncResolve("www.named-data.net", "6363",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v4(), 6363), true, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, false));

  TcpResolver::asyncResolve("www.named-data.net", "notport",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v4(), 0), false, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, true)); // should fail


  TcpResolver::asyncResolve("nothost.nothost.nothost.arpa", "6363",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v4(), 6363), false, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, true)); // should fail

  TcpResolver::asyncResolve("www.google.com", "80",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v4(), 80), true, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, false),
                            resolver::Ipv4Address()); // request IPv4 address

  TcpResolver::asyncResolve("www.google.com", "80",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, false),
                            resolver::Ipv6Address()); // request IPv6 address

  TcpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, false));

  TcpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, false),
                            resolver::Ipv6Address());

  TcpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v6(), 80), false, false),
                            bind(&ResolverFixture<tcp>::onFailure, this, true), // should fail
                            resolver::Ipv4Address());

  TcpResolver::asyncResolve("192.0.2.1", "80",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v4::from_string("192.0.2.1"), 80), true, true),
                            bind(&ResolverFixture<tcp>::onFailure, this, false));

  TcpResolver::asyncResolve("2001:db8:3f9:0:3025:ccc5:eeeb:86d3", "80",
                            bind(&ResolverFixture<tcp>::onSuccess, this, _1,
                                 tcp::endpoint(address_v6::
                                               from_string("2001:db8:3f9:0:3025:ccc5:eeeb:86d3"),
                                               80), true, true),
                            bind(&ResolverFixture<tcp>::onFailure, this, false));

  g_io.run();

  BOOST_CHECK_EQUAL(m_nFailures, 3);
  BOOST_CHECK_EQUAL(m_nSuccesses, 7);
}

BOOST_AUTO_TEST_CASE(SyncTcp)
{
  tcp::endpoint endpoint;
  BOOST_CHECK_NO_THROW(endpoint = TcpResolver::syncResolve("www.named-data.net", "6363"));
  NFD_LOG_DEBUG("Resolved to: " << endpoint);
  BOOST_CHECK_EQUAL(endpoint.address().is_v4(), true);
  BOOST_CHECK_EQUAL(endpoint.port(), 6363);
}

BOOST_FIXTURE_TEST_CASE(Udp, ResolverFixture<udp>)
{
  UdpResolver::asyncResolve("www.named-data.net", "6363",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v4(), 6363), true, false),
                            bind(&ResolverFixture<udp>::onFailure, this, false));

  UdpResolver::asyncResolve("www.named-data.net", "notport",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v4(), 0), false, false),
                            bind(&ResolverFixture<udp>::onFailure, this, true)); // should fail


  UdpResolver::asyncResolve("nothost.nothost.nothost.arpa", "6363",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v4(), 6363), false, false),
                            bind(&ResolverFixture<udp>::onFailure, this, true)); // should fail

  UdpResolver::asyncResolve("www.google.com", "80",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v4(), 80), true, false),
                            bind(&ResolverFixture<udp>::onFailure, this, false),
                            resolver::Ipv4Address()); // request IPv4 address

  UdpResolver::asyncResolve("www.google.com", "80",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<udp>::onFailure, this, false),
                            resolver::Ipv6Address()); // request IPv6 address

  UdpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<udp>::onFailure, this, false));

  UdpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v6(), 80), true, false),
                            bind(&ResolverFixture<udp>::onFailure, this, false),
                            resolver::Ipv6Address());

  UdpResolver::asyncResolve("ipv6.google.com", "80", // only IPv6 address should be available
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v6(), 80), false, false),
                            bind(&ResolverFixture<udp>::onFailure, this, true), // should fail
                            resolver::Ipv4Address());

  UdpResolver::asyncResolve("192.0.2.1", "80",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v4::from_string("192.0.2.1"), 80), true, true),
                            bind(&ResolverFixture<udp>::onFailure, this, false));

  UdpResolver::asyncResolve("2001:db8:3f9:0:3025:ccc5:eeeb:86d3", "80",
                            bind(&ResolverFixture<udp>::onSuccess, this, _1,
                                 udp::endpoint(address_v6::
                                               from_string("2001:db8:3f9:0:3025:ccc5:eeeb:86d3"),
                                               80), true, true),
                            bind(&ResolverFixture<udp>::onFailure, this, false));

  g_io.run();

  BOOST_CHECK_EQUAL(m_nFailures, 3);
  BOOST_CHECK_EQUAL(m_nSuccesses, 7);
}

BOOST_AUTO_TEST_CASE(SyncUdp)
{
  udp::endpoint endpoint;
  BOOST_CHECK_NO_THROW(endpoint = UdpResolver::syncResolve("www.named-data.net", "6363"));
  NFD_LOG_DEBUG("Resolved to: " << endpoint);
  BOOST_CHECK_EQUAL(endpoint.address().is_v4(), true);
  BOOST_CHECK_EQUAL(endpoint.port(), 6363);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
