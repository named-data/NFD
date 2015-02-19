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

#ifndef NFD_TESTS_NFD_MGMT_STRATEGY_CHOICE_PUBLISHER_HPP
#define NFD_TESTS_NFD_MGMT_STRATEGY_CHOICE_PUBLISHER_HPP

#include "mgmt/strategy-choice-publisher.hpp"
#include "mgmt/app-face.hpp"
#include "mgmt/internal-face.hpp"
#include "fw/forwarder.hpp"
#include "../fw/dummy-strategy.hpp"

#include "tests/test-common.hpp"

#include <ndn-cxx/management/nfd-strategy-choice.hpp>

namespace nfd {
namespace tests {

class StrategyChoicePublisherFixture : BaseFixture
{
public:

  StrategyChoicePublisherFixture()
    : m_strategyChoice(m_forwarder.getStrategyChoice())
    , m_face(make_shared<InternalFace>())
    , m_publisher(m_strategyChoice, *m_face, "/localhost/nfd/strategy-choice/list", m_keyChain)
    , STRATEGY_A(make_shared<DummyStrategy>(boost::ref(m_forwarder),
                                            "/localhost/nfd/strategy/dummy-strategy-a"))
    , STRATEGY_B(make_shared<DummyStrategy>(boost::ref(m_forwarder),
                                            "/localhost/nfd/strategy/dummy-strategy-b"))
    , m_finished(false)
  {
    m_strategyChoice.install(STRATEGY_A);
    m_strategyChoice.install(STRATEGY_B);

    ndn::nfd::StrategyChoice expectedRootEntry;
    expectedRootEntry.setStrategy(STRATEGY_A->getName());
    expectedRootEntry.setName("/");

    m_strategyChoice.insert("/", STRATEGY_A->getName());
    m_expectedEntries["/"] = expectedRootEntry;
  }

  void
  validatePublish(const Data& data)
  {
    Block payload = data.getContent();

    m_buffer.appendByteArray(payload.value(), payload.value_size());

    BOOST_CHECK_NO_THROW(data.getName()[-1].toSegment());
    if (data.getFinalBlockId() != data.getName()[-1])
      {
        return;
      }

    // wrap the Strategy Choice entries in a single Content TLV for easy parsing
    m_buffer.prependVarNumber(m_buffer.size());
    m_buffer.prependVarNumber(tlv::Content);

    ndn::Block parser(m_buffer.buf(), m_buffer.size());
    parser.parse();

    BOOST_REQUIRE_EQUAL(parser.elements_size(), m_expectedEntries.size());

    for (Block::element_const_iterator i = parser.elements_begin();
         i != parser.elements_end();
         ++i)
      {
        if (i->type() != ndn::tlv::nfd::StrategyChoice)
          {
            BOOST_FAIL("expected StrategyChoice, got type #" << i->type());
          }

        ndn::nfd::StrategyChoice entry(*i);

        std::map<std::string, ndn::nfd::StrategyChoice>::const_iterator expectedEntryPos =
          m_expectedEntries.find(entry.getName().toUri());

        BOOST_REQUIRE(expectedEntryPos != m_expectedEntries.end());
        const ndn::nfd::StrategyChoice& expectedEntry = expectedEntryPos->second;

        BOOST_CHECK_EQUAL(entry.getStrategy(), expectedEntry.getStrategy());
        BOOST_CHECK_EQUAL(entry.getName(), expectedEntry.getName());

        m_matchedEntries.insert(entry.getName().toUri());
      }

    BOOST_CHECK_EQUAL(m_matchedEntries.size(), m_expectedEntries.size());

    m_finished = true;
  }

protected:
  Forwarder m_forwarder;
  StrategyChoice& m_strategyChoice;
  shared_ptr<InternalFace> m_face;
  StrategyChoicePublisher m_publisher;

  shared_ptr<DummyStrategy> STRATEGY_A;
  shared_ptr<DummyStrategy> STRATEGY_B;

  ndn::EncodingBuffer m_buffer;

  std::map<std::string, ndn::nfd::StrategyChoice> m_expectedEntries;
  std::set<std::string> m_matchedEntries;

  bool m_finished;

  ndn::KeyChain m_keyChain;
};



BOOST_FIXTURE_TEST_SUITE(MgmtStrategyChoicePublisher, StrategyChoicePublisherFixture)

BOOST_AUTO_TEST_CASE(Publish)
{
  m_strategyChoice.insert("/test/a", STRATEGY_A->getName());
  m_strategyChoice.insert("/test/b", STRATEGY_B->getName());

  ndn::nfd::StrategyChoice expectedEntryA;
  expectedEntryA.setStrategy(STRATEGY_A->getName());
  expectedEntryA.setName("/test/a");

  ndn::nfd::StrategyChoice expectedEntryB;
  expectedEntryB.setStrategy(STRATEGY_B->getName());
  expectedEntryB.setName("/test/b");

  m_expectedEntries["/test/a"] = expectedEntryA;
  m_expectedEntries["/test/b"] = expectedEntryB;

  m_face->onReceiveData.connect(bind(&StrategyChoicePublisherFixture::validatePublish, this, _1));

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}


BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_NFD_MGMT_STRATEGY_CHOICE_PUBLISHER_HPP
