/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/fib-enumeration-publisher.hpp"

#include "mgmt/app-face.hpp"
#include "mgmt/internal-face.hpp"

#include "tests/test-common.hpp"
#include "../face/dummy-face.hpp"

#include "fib-enumeration-publisher-common.hpp"

#include <ndn-cpp-dev/encoding/tlv.hpp>

namespace nfd {
namespace tests {

NFD_LOG_INIT("TestFibEnumerationPublisher");



BOOST_FIXTURE_TEST_SUITE(MgmtFibEnumeration, FibEnumerationPublisherFixture)

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

  m_face->onReceiveData +=
    bind(&FibEnumerationPublisherFixture::decodeFibEntryBlock, this, _1);

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
