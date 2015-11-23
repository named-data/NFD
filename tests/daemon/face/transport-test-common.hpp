/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

namespace nfd {
namespace face {
namespace tests {

/** \brief check a transport has all its static properties set after initialization
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

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_TRANSPORT_TEST_COMMON_HPP
