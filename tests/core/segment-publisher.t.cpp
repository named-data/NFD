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

#include "core/segment-publisher.hpp"
#include <ndn-cxx/encoding/tlv.hpp>

#include "tests/test-common.hpp"
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace tests {

NFD_LOG_INIT("SegmentPublisherTest");

template<int64_t N=10000>
class SegmentPublisherTester : public SegmentPublisher<ndn::util::DummyClientFace>
{
public:
  SegmentPublisherTester(ndn::util::DummyClientFace& face,
                       const Name& prefix,
                       ndn::KeyChain& keyChain,
                       const time::milliseconds freshnessPeriod)
    : SegmentPublisher(face, prefix, keyChain, freshnessPeriod)
    , m_totalPayloadLength(0)
  {

  }

  virtual
  ~SegmentPublisherTester()
  {
  }

  uint16_t
  getLimit() const
  {
    return N;
  }

  size_t
  getTotalPayloadLength() const
  {
    return m_totalPayloadLength;
  }

protected:

  virtual size_t
  generate(ndn::EncodingBuffer& outBuffer)
  {
    size_t totalLength = 0;
    for (int64_t i = 0; i < N; i++)
      {
        totalLength += prependNonNegativeIntegerBlock(outBuffer, tlv::Content, i);
      }
    m_totalPayloadLength += totalLength;
    return totalLength;
  }

protected:
  size_t m_totalPayloadLength;
};

template<int64_t N>
class SegmentPublisherFixture : public BaseFixture
{
public:
  SegmentPublisherFixture()
    : m_face(ndn::util::makeDummyClientFace())
    , m_expectedFreshnessPeriod(time::milliseconds(111))
    , m_publisher(*m_face, "/localhost/nfd/SegmentPublisherFixture",
                  m_keyChain, m_expectedFreshnessPeriod)
  {
  }

  void
  validate(const Data& data)
  {
    BOOST_CHECK_EQUAL(data.getFreshnessPeriod(), m_expectedFreshnessPeriod);

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
    m_buffer.prependVarNumber(tlv::Content);

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
  }

protected:
  shared_ptr<ndn::util::DummyClientFace> m_face;
  const time::milliseconds m_expectedFreshnessPeriod;
  SegmentPublisherTester<N> m_publisher;
  ndn::EncodingBuffer m_buffer;
  ndn::KeyChain m_keyChain;
};

using boost::mpl::int_;
typedef boost::mpl::vector<int_<10000>, int_<100>, int_<10>, int_<0> > DatasetSizes;

BOOST_AUTO_TEST_SUITE(TestSegmentPublisher)

BOOST_FIXTURE_TEST_CASE_TEMPLATE(Generate, T, DatasetSizes, SegmentPublisherFixture<T::value>)
{
  this->m_publisher.publish();
  this->m_face->processEvents();

  size_t nSegments = this->m_publisher.getTotalPayloadLength() /
                     this->m_publisher.getMaxSegmentSize();
  if (this->m_publisher.getTotalPayloadLength() % this->m_publisher.getMaxSegmentSize() != 0 ||
      nSegments == 0)
    ++nSegments;

  BOOST_CHECK_EQUAL(this->m_face->sentDatas.size(), nSegments);
  for (const Data& data : this->m_face->sentDatas) {
    this->validate(data);
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
