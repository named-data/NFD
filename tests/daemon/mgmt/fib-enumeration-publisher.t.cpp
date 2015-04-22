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

#include "mgmt/fib-enumeration-publisher.hpp"

#include "mgmt/app-face.hpp"
#include "mgmt/internal-face.hpp"

#include "tests/test-common.hpp"
#include "../face/dummy-face.hpp"

#include "fib-enumeration-publisher-common.hpp"

#include <ndn-cxx/encoding/tlv.hpp>

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(MgmtFibEnumerationPublisher, FibEnumerationPublisherFixture)

BOOST_AUTO_TEST_CASE(TestFibEnumerationPublisher)
{
  for (int i = 0; i < 87; i++)
    {
      Name prefix("/test");
      prefix.appendSegment(i);

      shared_ptr<DummyFace> dummy1(make_shared<DummyFace>());
      shared_ptr<DummyFace> dummy2(make_shared<DummyFace>());

      shared_ptr<fib::Entry> entry = m_fib.insert(prefix).first;
      entry->addNextHop(dummy1, std::numeric_limits<uint64_t>::max() - 1);
      entry->addNextHop(dummy2, std::numeric_limits<uint64_t>::max() - 2);

      m_referenceEntries.insert(entry);
    }
  for (int i = 0; i < 2; i++)
    {
      Name prefix("/test2");
      prefix.appendSegment(i);

      shared_ptr<DummyFace> dummy1(make_shared<DummyFace>());
      shared_ptr<DummyFace> dummy2(make_shared<DummyFace>());

      shared_ptr<fib::Entry> entry = m_fib.insert(prefix).first;
      entry->addNextHop(dummy1, std::numeric_limits<uint8_t>::max() - 1);
      entry->addNextHop(dummy2, std::numeric_limits<uint8_t>::max() - 2);

      m_referenceEntries.insert(entry);
    }

  ndn::EncodingBuffer buffer;

  m_face->onReceiveData.connect(
      bind(&FibEnumerationPublisherFixture::decodeFibEntryBlock, this, _1));

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
