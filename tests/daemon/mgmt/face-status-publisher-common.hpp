/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_TESTS_NFD_MGMT_FACE_STATUS_PUBLISHER_COMMON_HPP
#define NFD_TESTS_NFD_MGMT_FACE_STATUS_PUBLISHER_COMMON_HPP

#include "mgmt/face-status-publisher.hpp"
#include "mgmt/app-face.hpp"
#include "mgmt/internal-face.hpp"
#include "fw/forwarder.hpp"

#include "tests/test-common.hpp"
#include "tests/daemon/face/dummy-face.hpp"

#include <ndn-cxx/management/nfd-face-status.hpp>

namespace nfd {
namespace tests {

class TestCountersFace : public DummyFace
{
public:

  TestCountersFace()
  {
  }

  virtual
  ~TestCountersFace()
  {
  }

  void
  setCounters(PacketCounter::rep nInInterests,
              PacketCounter::rep nInDatas,
              PacketCounter::rep nOutInterests,
              PacketCounter::rep nOutDatas,
              ByteCounter::rep nInBytes,
              ByteCounter::rep nOutBytes)
  {
    FaceCounters& counters = getMutableCounters();
    counters.getNInInterests().set(nInInterests);
    counters.getNInDatas().set(nInDatas);
    counters.getNOutInterests().set(nOutInterests);
    counters.getNOutDatas().set(nOutDatas);
    counters.getNInBytes().set(nInBytes);
    counters.getNOutBytes().set(nOutBytes);
  }


};

static inline uint64_t
readNonNegativeIntegerType(const Block& block,
                           uint32_t type)
{
  if (block.type() == type)
    {
      return readNonNegativeInteger(block);
    }
  std::stringstream error;
  error << "expected type " << type << " got " << block.type();
  BOOST_THROW_EXCEPTION(tlv::Error(error.str()));
}

static inline uint64_t
checkedReadNonNegativeIntegerType(Block::element_const_iterator& i,
                                  Block::element_const_iterator end,
                                  uint32_t type)
{
  if (i != end)
    {
      const Block& block = *i;
      ++i;
      return readNonNegativeIntegerType(block, type);
    }
  BOOST_THROW_EXCEPTION(tlv::Error("Unexpected end of FaceStatus"));
}

class FaceStatusPublisherFixture : public BaseFixture
{
public:

  FaceStatusPublisherFixture()
    : m_table(m_forwarder)
    , m_face(make_shared<InternalFace>())
    , m_publisher(m_table, *m_face, "/localhost/nfd/FaceStatusPublisherFixture", m_keyChain)
    , m_finished(false)
  {

  }

  virtual
  ~FaceStatusPublisherFixture()
  {

  }

  void
  add(shared_ptr<Face> face)
  {
    m_table.add(face);
  }

  void
  validateFaceStatus(const Block& statusBlock, const shared_ptr<Face>& reference)
  {
    ndn::nfd::FaceStatus status;
    BOOST_REQUIRE_NO_THROW(status.wireDecode(statusBlock));
    const FaceCounters& counters = reference->getCounters();

    BOOST_CHECK_EQUAL(status.getFaceId(), reference->getId());
    BOOST_CHECK_EQUAL(status.getRemoteUri(), reference->getRemoteUri().toString());
    BOOST_CHECK_EQUAL(status.getLocalUri(), reference->getLocalUri().toString());

    if (reference->isLocal()) {
      BOOST_CHECK_EQUAL(status.getFaceScope(), ndn::nfd::FACE_SCOPE_LOCAL);
    }
    else {
      BOOST_CHECK_EQUAL(status.getFaceScope(), ndn::nfd::FACE_SCOPE_NON_LOCAL);
    }

    BOOST_CHECK_EQUAL(status.getFacePersistency(), reference->getPersistency());

    if (reference->isMultiAccess()) {
      BOOST_CHECK_EQUAL(status.getLinkType(), ndn::nfd::LINK_TYPE_MULTI_ACCESS);
    }
    else {
      BOOST_CHECK_EQUAL(status.getLinkType(), ndn::nfd::LINK_TYPE_POINT_TO_POINT);
    }

    BOOST_CHECK_EQUAL(status.getNInInterests(), counters.getNInInterests());
    BOOST_CHECK_EQUAL(status.getNInDatas(), counters.getNInDatas());
    BOOST_CHECK_EQUAL(status.getNOutInterests(), counters.getNOutInterests());
    BOOST_CHECK_EQUAL(status.getNOutDatas(), counters.getNOutDatas());
    BOOST_CHECK_EQUAL(status.getNInBytes(), counters.getNInBytes());
    BOOST_CHECK_EQUAL(status.getNOutBytes(), counters.getNOutBytes());
  }

  void
  decodeFaceStatusBlock(const Data& data)
  {
    Block payload = data.getContent();

    m_buffer.appendByteArray(payload.value(), payload.value_size());

    BOOST_CHECK_NO_THROW(data.getName()[-1].toSegment());
    if (data.getFinalBlockId() != data.getName()[-1])
      {
        return;
      }

    // wrap the Face Statuses in a single Content TLV for easy parsing
    m_buffer.prependVarNumber(m_buffer.size());
    m_buffer.prependVarNumber(tlv::Content);

    ndn::Block parser(m_buffer.buf(), m_buffer.size());
    parser.parse();

    BOOST_REQUIRE_EQUAL(parser.elements_size(), m_referenceFaces.size());

    std::list<shared_ptr<Face> >::const_iterator iReference = m_referenceFaces.begin();
    for (Block::element_const_iterator i = parser.elements_begin();
         i != parser.elements_end();
         ++i)
      {
        if (i->type() != ndn::tlv::nfd::FaceStatus)
          {
            BOOST_FAIL("expected face status, got type #" << i->type());
          }
        validateFaceStatus(*i, *iReference);
        ++iReference;
      }
    m_finished = true;
  }

protected:
  Forwarder m_forwarder;
  FaceTable m_table;
  shared_ptr<InternalFace> m_face;
  FaceStatusPublisher m_publisher;
  ndn::EncodingBuffer m_buffer;
  std::list<shared_ptr<Face> > m_referenceFaces;
  ndn::KeyChain m_keyChain;

protected:
  bool m_finished;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_NFD_MGMT_FACE_STATUS_PUBLISHER_COMMON_HPP
