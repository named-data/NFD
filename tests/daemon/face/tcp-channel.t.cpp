/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "tcp-channel-fixture.hpp"

#include "test-ip.hpp"
#include <boost/mpl/vector.hpp>

namespace nfd {
namespace face {
namespace tests {

BOOST_AUTO_TEST_SUITE(Face)
BOOST_FIXTURE_TEST_SUITE(TestTcpChannel, TcpChannelFixture)

using AddressFamilies = boost::mpl::vector<
  std::integral_constant<AddressFamily, AddressFamily::V4>,
  std::integral_constant<AddressFamily, AddressFamily::V6>>;

BOOST_AUTO_TEST_CASE_TEMPLATE(ConnectTimeout, F, AddressFamilies)
{
  auto address = getTestIp(F::value, AddressScope::Loopback);
  SKIP_IF_IP_UNAVAILABLE(address);
  // do not listen

  auto channel = this->makeChannel(typename IpAddressFromFamily<F::value>::type());
  channel->connect(tcp::Endpoint(address, 7040), {},
    [this] (const shared_ptr<nfd::Face>&) {
      BOOST_FAIL("Connect succeeded when it should have failed");
      this->limitedIo.afterOp();
    },
    [this] (uint32_t, const std::string& reason) {
      BOOST_CHECK_EQUAL(reason.empty(), false);
      this->limitedIo.afterOp();
    },
    time::seconds(1));

  BOOST_CHECK_EQUAL(this->limitedIo.run(1, time::seconds(2)), LimitedIo::EXCEED_OPS);
  BOOST_CHECK_EQUAL(channel->size(), 0);
}

BOOST_AUTO_TEST_SUITE_END() // TestTcpChannel
BOOST_AUTO_TEST_SUITE_END() // Face

} // namespace tests
} // namespace face
} // namespace nfd
