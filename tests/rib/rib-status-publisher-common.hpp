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

#ifndef RIB_TESTS_UNIT_TESTS_RIB_STATUS_PUBLISHER_COMMON_HPP
#define RIB_TESTS_UNIT_TESTS_RIB_STATUS_PUBLISHER_COMMON_HPP

#include "rib/rib-status-publisher.hpp"

#include "tests/test-common.hpp"
#include "rib/rib.hpp"

#include <ndn-cxx/management/nfd-control-parameters.hpp>
#include <ndn-cxx/management/nfd-rib-entry.hpp>
#include <ndn-cxx/encoding/tlv.hpp>

namespace nfd {
namespace rib {
namespace tests {

using ndn::nfd::ControlParameters;

class RibStatusPublisherFixture : public nfd::tests::BaseFixture
{
public:
  static void
  validateRibEntry(const Block& block, const Name& referenceName, const Route& referenceRoute)
  {
    ndn::nfd::RibEntry entry;
    BOOST_REQUIRE_NO_THROW(entry.wireDecode(block));

    BOOST_CHECK_EQUAL(entry.getName(), referenceName);

    std::list<ndn::nfd::Route> routes = entry.getRoutes();

    std::list<ndn::nfd::Route>::iterator it = routes.begin();
    BOOST_CHECK_EQUAL(it->getFaceId(), referenceRoute.faceId);
    BOOST_CHECK_EQUAL(it->getOrigin(), referenceRoute.origin);
    BOOST_CHECK_EQUAL(it->getCost(), referenceRoute.cost);
    BOOST_CHECK_EQUAL(it->getFlags(), referenceRoute.flags);
  }

  static void
  decodeRibEntryBlock(const Data& data, const Name& referenceName, const Route& referenceRoute)
  {
    ndn::EncodingBuffer buffer;

    Block payload = data.getContent();

    buffer.appendByteArray(payload.value(), payload.value_size());
    buffer.prependVarNumber(buffer.size());
    buffer.prependVarNumber(tlv::Content);

    ndn::Block parser(buffer.buf(), buffer.size());
    parser.parse();

    Block::element_const_iterator i = parser.elements_begin();

    if (i->type() != ndn::tlv::nfd::RibEntry) {
      BOOST_FAIL("expected RibEntry, got type #" << i->type());
    }
    else {
      validateRibEntry(*i, referenceName, referenceRoute);
    }
  }
};

#endif // RIB_TESTS_UNIT_TESTS_RIB_STATUS_PUBLISHER_COMMON_HPP

} // namespace tests
} // namespace rib
} // namespace nfd
