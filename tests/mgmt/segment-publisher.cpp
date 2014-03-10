/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "mgmt/segment-publisher.hpp"
#include "mgmt/internal-face.hpp"
#include "mgmt/app-face.hpp"

#include "tests/test-common.hpp"
#include <ndn-cpp-dev/encoding/tlv.hpp>

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
