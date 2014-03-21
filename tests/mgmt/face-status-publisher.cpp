/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face-status-publisher-common.hpp"

namespace nfd {
namespace tests {

NFD_LOG_INIT("FaceStatusPublisherTest");

BOOST_FIXTURE_TEST_SUITE(MgmtFaceSatusPublisher, FaceStatusPublisherFixture)

BOOST_AUTO_TEST_CASE(EncodingDecoding)
{
  Name commandName("/localhost/nfd/faces/list");
  shared_ptr<Interest> command(make_shared<Interest>(commandName));

  // MAX_SEGMENT_SIZE == 4400, FaceStatus size with filler counters is 55
  // 55 divides 4400 (== 80), so only use 79 FaceStatuses and then two smaller ones
  // to force a FaceStatus to span Data packets
  for (int i = 0; i < 79; i++)
    {
      shared_ptr<TestCountersFace> dummy(make_shared<TestCountersFace>());

      uint64_t filler = std::numeric_limits<uint64_t>::max() - 1;
      dummy->setCounters(filler, filler, filler, filler);

      m_referenceFaces.push_back(dummy);

      add(dummy);
    }

  for (int i = 0; i < 2; i++)
    {
      shared_ptr<TestCountersFace> dummy(make_shared<TestCountersFace>());
      uint64_t filler = std::numeric_limits<uint32_t>::max() - 1;
      dummy->setCounters(filler, filler, filler, filler);

      m_referenceFaces.push_back(dummy);

      add(dummy);
    }

  ndn::EncodingBuffer buffer;

  m_face->onReceiveData +=
    bind(&FaceStatusPublisherFixture::decodeFaceStatusBlock, this, _1);

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
