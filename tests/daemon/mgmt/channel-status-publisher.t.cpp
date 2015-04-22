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

#include "mgmt/channel-status-publisher.hpp"
#include "mgmt/internal-face.hpp"

#include "channel-status-common.hpp"

namespace nfd {
namespace tests {

class ChannelStatusPublisherFixture : BaseFixture
{
public:
  ChannelStatusPublisherFixture()
    : m_face(make_shared<InternalFace>())
    , m_publisher(m_factories, *m_face, "/localhost/nfd/faces/channels", m_keyChain)
    , m_finished(false)
  {
  }

  virtual
  ~ChannelStatusPublisherFixture()
  {
  }

  // virtual shared_ptr<DummyProtocolFactory>
  // addProtocolFactory(const std::string& protocol)
  // {
  //   shared_ptr<DummyProtocolFactory> factory(make_shared<DummyProtocolFactory>());
  //   m_factories[protocol] = factory;

  //   return factory;
  // }

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

    // wrap the Channel Status entries in a single Content TLV for easy parsing
    m_buffer.prependVarNumber(m_buffer.size());
    m_buffer.prependVarNumber(tlv::Content);

    ndn::Block parser(m_buffer.buf(), m_buffer.size());
    parser.parse();

    BOOST_REQUIRE_EQUAL(parser.elements_size(), m_expectedEntries.size());

    for (Block::element_const_iterator i = parser.elements_begin();
         i != parser.elements_end();
         ++i)
      {
        if (i->type() != ndn::tlv::nfd::ChannelStatus)
          {
            BOOST_FAIL("expected ChannelStatus, got type #" << i->type());
          }

        ndn::nfd::ChannelStatus entry(*i);

        std::map<std::string, ndn::nfd::ChannelStatus>::const_iterator expectedEntryPos =
          m_expectedEntries.find(entry.getLocalUri());

        BOOST_REQUIRE(expectedEntryPos != m_expectedEntries.end());
        const ndn::nfd::ChannelStatus& expectedEntry = expectedEntryPos->second;

        BOOST_CHECK_EQUAL(entry.getLocalUri(), expectedEntry.getLocalUri());

        m_matchedEntries.insert(entry.getLocalUri());
      }

    BOOST_CHECK_EQUAL(m_matchedEntries.size(), m_expectedEntries.size());

    m_finished = true;
  }

protected:
  ChannelStatusPublisher::FactoryMap m_factories;
  shared_ptr<InternalFace> m_face;
  ChannelStatusPublisher m_publisher;

  ndn::EncodingBuffer m_buffer;

  std::map<std::string, ndn::nfd::ChannelStatus> m_expectedEntries;
  std::set<std::string> m_matchedEntries;

  bool m_finished;

  ndn::KeyChain m_keyChain;
};

BOOST_FIXTURE_TEST_SUITE(MgmtChannelStatusPublisher, ChannelStatusPublisherFixture)

BOOST_AUTO_TEST_CASE(Publish)
{
  const std::string protocol = "dummy";

  shared_ptr<DummyProtocolFactory> factory(make_shared<DummyProtocolFactory>());
  m_factories[protocol] = factory;

  for (int i = 0; i < 10; ++i)
    {
      const std::string uri = protocol + "://path" + boost::lexical_cast<std::string>(i);
      factory->addChannel(uri);

      ndn::nfd::ChannelStatus expectedEntry;
      expectedEntry.setLocalUri(DummyChannel(uri).getUri().toString());

      m_expectedEntries[expectedEntry.getLocalUri()] = expectedEntry;
    }

  m_face->onReceiveData.connect(bind(&ChannelStatusPublisherFixture::validatePublish, this, _1));

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_CASE(DuplicateFactories)
{
  const std::string protocol1 = "dummy1";
  const std::string protocol2 = "dummy2";

  shared_ptr<DummyProtocolFactory> factory(make_shared<DummyProtocolFactory>());
  m_factories[protocol1] = factory;
  m_factories[protocol2] = factory;

  for (int i = 0; i < 10; ++i)
    {
      ndn::nfd::ChannelStatus expectedEntry;
      const std::string uri = protocol1 + "://path" + boost::lexical_cast<std::string>(i);

      factory->addChannel(uri);

      expectedEntry.setLocalUri(DummyChannel(uri).getUri().toString());
      m_expectedEntries[expectedEntry.getLocalUri()] = expectedEntry;
    }

  m_face->onReceiveData.connect(bind(&ChannelStatusPublisherFixture::validatePublish, this, _1));

  m_publisher.publish();
  BOOST_REQUIRE(m_finished);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests



} // namespace nfd
