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

#include "mgmt/segment-publisher.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/app-face.hpp"

#include "tests/test-common.hpp"
#include <ndn-cxx/encoding/tlv.hpp>

namespace nfd {
namespace tests {

NFD_LOG_INIT("SegmentPublisherTest");

class TestSegmentPublisher : public SegmentPublisher
{
public:
  TestSegmentPublisher(shared_ptr<AppFace> face,
                       const Name& prefix,
                       const uint64_t limit=10000)
    : SegmentPublisher(face, prefix)
    , m_limit((limit == 0)?(1):(limit))
  {

  }

  virtual
  ~TestSegmentPublisher()
  {

  }

  uint16_t
  getLimit() const
  {
    return m_limit;
  }

protected:

  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer)
  {
    size_t totalLength = 0;
    for (uint64_t i = 0; i < m_limit; i++)
      {
        totalLength += prependNonNegativeIntegerBlock(outBuffer, ndn::Tlv::Content, i);
      }
    return totalLength;
  }

protected:
  const uint64_t m_limit;
};

class SegmentPublisherFixture : public BaseFixture
{
public:
  SegmentPublisherFixture()
    : m_face(make_shared<InternalFace>())
    , m_publisher(m_face, "/localhost/nfd/SegmentPublisherFixture")
    , m_finished(false)
  {

  }

  void
  validate(const Data& data)
  {
    Block payload = data.getContent();
    NFD_LOG_DEBUG("payload size (w/o Content TLV): " << payload.value_size());

    m_buffer.appendByteArray(payload.value(), payload.value_size());

    uint64_t segmentNo = data.getName()[-1].toSegment();
    if (data.getFinalBlockId() != data.getName()[-1])
      {
        return;
      }

    NFD_LOG_DEBUG("got final block: #" << segmentNo);

    // wrap data in a single Content TLV for easy parsing
    m_buffer.prependVarNumber(m_buffer.size());
    m_buffer.prependVarNumber(ndn::Tlv::Content);

    BOOST_TEST_CHECKPOINT("creating parser");
    ndn::Block parser(m_buffer.buf(), m_buffer.size());
    BOOST_TEST_CHECKPOINT("parsing aggregated response");
    parser.parse();

    BOOST_REQUIRE_EQUAL(parser.elements_size(), m_publisher.getLimit());

    uint64_t expectedNo = m_publisher.getLimit() - 1;
    for (Block::element_const_iterator i = parser.elements_begin();
         i != parser.elements_end();
         ++i)
      {
        uint64_t number = readNonNegativeInteger(*i);
        BOOST_REQUIRE_EQUAL(number, expectedNo);
        --expectedNo;
      }
    m_finished = true;
  }

protected:
  shared_ptr<InternalFace> m_face;
  TestSegmentPublisher m_publisher;
  ndn::EncodingBuffer m_buffer;

protected:
  bool m_finished;
};

BOOST_FIXTURE_TEST_SUITE(MgmtSegmentPublisher, SegmentPublisherFixture)

BOOST_AUTO_TEST_CASE(Generate)
{
  m_face->onReceiveData +=
    bind(&SegmentPublisherFixture::validate, this, _1);

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
