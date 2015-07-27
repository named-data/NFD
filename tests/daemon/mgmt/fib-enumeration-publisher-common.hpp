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

#ifndef NFD_TESTS_NFD_MGMT_FIB_ENUMERATION_PUBLISHER_COMMON_HPP
#define NFD_TESTS_NFD_MGMT_FIB_ENUMERATION_PUBLISHER_COMMON_HPP

#include "mgmt/fib-enumeration-publisher.hpp"

#include "mgmt/app-face.hpp"
#include "mgmt/internal-face.hpp"
#include "table/fib.hpp"
#include "table/name-tree.hpp"

#include "tests/test-common.hpp"
#include "../face/dummy-face.hpp"

#include <ndn-cxx/encoding/tlv.hpp>

namespace nfd {
namespace tests {

static inline uint64_t
readNonNegativeIntegerType(const Block& block,
                           uint32_t type)
{
  if (block.type() == type)
    {
      return readNonNegativeInteger(block);
    }
  std::stringstream error;
  error << "Expected type " << type << " got " << block.type();
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
  std::stringstream error;
  error << "Unexpected end of Block while attempting to read type #"
        << type;
  BOOST_THROW_EXCEPTION(tlv::Error(error.str()));
}

class FibEnumerationPublisherFixture : public BaseFixture
{
public:

  FibEnumerationPublisherFixture()
    : m_fib(m_nameTree)
    , m_face(make_shared<InternalFace>())
    , m_publisher(m_fib, *m_face, "/localhost/nfd/FibEnumerationPublisherFixture", m_keyChain)
    , m_finished(false)
  {
  }

  virtual
  ~FibEnumerationPublisherFixture()
  {
  }

  bool
  hasNextHopWithCost(const fib::NextHopList& nextHops,
                     FaceId faceId,
                     uint64_t cost)
  {
    for (fib::NextHopList::const_iterator i = nextHops.begin();
         i != nextHops.end();
         ++i)
      {
        if (i->getFace()->getId() == faceId && i->getCost() == cost)
          {
            return true;
          }
      }
    return false;
  }

  bool
  entryHasPrefix(const shared_ptr<fib::Entry> entry, const Name& prefix)
  {
    return entry->getPrefix() == prefix;
  }

  void
  validateFibEntry(const Block& entry)
  {
    entry.parse();

    Block::element_const_iterator i = entry.elements_begin();
    BOOST_REQUIRE(i != entry.elements_end());


    BOOST_REQUIRE(i->type() == tlv::Name);
    Name prefix(*i);
    ++i;

    std::set<shared_ptr<fib::Entry> >::const_iterator referenceIter =
      std::find_if(m_referenceEntries.begin(), m_referenceEntries.end(),
                   bind(&FibEnumerationPublisherFixture::entryHasPrefix,
                        this, _1, prefix));

    BOOST_REQUIRE(referenceIter != m_referenceEntries.end());

    const shared_ptr<fib::Entry>& reference = *referenceIter;
    BOOST_REQUIRE_EQUAL(prefix, reference->getPrefix());

    // 0 or more next hop records
    size_t nRecords = 0;
    const fib::NextHopList& referenceNextHops = reference->getNextHops();
    for (; i != entry.elements_end(); ++i)
      {
        const ndn::Block& nextHopRecord = *i;
        BOOST_REQUIRE(nextHopRecord.type() == ndn::tlv::nfd::NextHopRecord);
        nextHopRecord.parse();

        Block::element_const_iterator j = nextHopRecord.elements_begin();

        FaceId faceId =
          checkedReadNonNegativeIntegerType(j,
                                            entry.elements_end(),
                                            ndn::tlv::nfd::FaceId);

        uint64_t cost =
          checkedReadNonNegativeIntegerType(j,
                                            entry.elements_end(),
                                            ndn::tlv::nfd::Cost);

        BOOST_REQUIRE(hasNextHopWithCost(referenceNextHops, faceId, cost));

        BOOST_REQUIRE(j == nextHopRecord.elements_end());
        nRecords++;
      }
    BOOST_REQUIRE_EQUAL(nRecords, referenceNextHops.size());

    BOOST_REQUIRE(i == entry.elements_end());
    m_referenceEntries.erase(referenceIter);
  }

  void
  decodeFibEntryBlock(const Data& data)
  {
    Block payload = data.getContent();

    m_buffer.appendByteArray(payload.value(), payload.value_size());

    BOOST_CHECK_NO_THROW(data.getName()[-1].toSegment());
    if (data.getFinalBlockId() != data.getName()[-1])
      {
        return;
      }

    // wrap the FIB Entry blocks in a single Content TLV for easy parsing
    m_buffer.prependVarNumber(m_buffer.size());
    m_buffer.prependVarNumber(tlv::Content);

    ndn::Block parser(m_buffer.buf(), m_buffer.size());
    parser.parse();

    BOOST_REQUIRE_EQUAL(parser.elements_size(), m_referenceEntries.size());

    for (Block::element_const_iterator i = parser.elements_begin();
         i != parser.elements_end();
         ++i)
      {
        if (i->type() != ndn::tlv::nfd::FibEntry)
          {
            BOOST_FAIL("expected fib entry, got type #" << i->type());
          }

        validateFibEntry(*i);
      }
    m_finished = true;
  }

protected:
  NameTree m_nameTree;
  Fib m_fib;
  shared_ptr<InternalFace> m_face;
  FibEnumerationPublisher m_publisher;
  ndn::EncodingBuffer m_buffer;
  std::set<shared_ptr<fib::Entry> > m_referenceEntries;
  ndn::KeyChain m_keyChain;

protected:
  bool m_finished;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_NFD_MGMT_FIB_ENUMERATION_PUBLISHER_COMMON_HPP
