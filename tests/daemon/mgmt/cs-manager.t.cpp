/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "mgmt/cs-manager.hpp"
#include "nfd-manager-common-fixture.hpp"
#include <ndn-cxx/mgmt/nfd/cs-info.hpp>

namespace nfd {
namespace tests {

class CsManagerFixture : public NfdManagerCommonFixture
{
public:
  CsManagerFixture()
    : m_cs(m_forwarder.getCs())
    , m_fwCnt(const_cast<ForwarderCounters&>(m_forwarder.getCounters()))
    , m_manager(m_cs, m_fwCnt, m_dispatcher, *m_authenticator)
  {
    setTopPrefix();
    setPrivilege("cs");
  }

protected:
  Cs& m_cs;
  ForwarderCounters& m_fwCnt;
  CsManager m_manager;
};

BOOST_AUTO_TEST_SUITE(Mgmt)
BOOST_FIXTURE_TEST_SUITE(TestCsManager, CsManagerFixture)

BOOST_AUTO_TEST_CASE(Config)
{
  using ndn::nfd::CsFlagBit;
  const Name cmdPrefix("/localhost/nfd/cs/config");

  // setup initial CS config
  m_cs.setLimit(22129);
  m_cs.enableAdmit(false);
  m_cs.enableServe(true);

  // send empty cs/config command
  auto req = makeControlCommandRequest(cmdPrefix, ControlParameters());
  receiveInterest(req);

  // response shall reflect current config
  ControlParameters body;
  body.setCapacity(22129);
  body.setFlagBit(CsFlagBit::BIT_CS_ENABLE_ADMIT, false, false);
  body.setFlagBit(CsFlagBit::BIT_CS_ENABLE_SERVE, true, false);
  BOOST_CHECK_EQUAL(checkResponse(0, req.getName(),
                                  ControlResponse(200, "OK").setBody(body.wireEncode())),
                    CheckResponseResult::OK);

  // send filled cs/config command
  ControlParameters parameters;
  parameters.setCapacity(18609);
  parameters.setFlagBit(CsFlagBit::BIT_CS_ENABLE_ADMIT, true);
  parameters.setFlagBit(CsFlagBit::BIT_CS_ENABLE_SERVE, false);
  req = makeControlCommandRequest(cmdPrefix, parameters);
  receiveInterest(req);

  // response shall reflect updated config
  body.setCapacity(18609);
  body.setFlagBit(CsFlagBit::BIT_CS_ENABLE_ADMIT, true, false);
  body.setFlagBit(CsFlagBit::BIT_CS_ENABLE_SERVE, false, false);
  BOOST_CHECK_EQUAL(checkResponse(1, req.getName(),
                                  ControlResponse(200, "OK").setBody(body.wireEncode())),
                    CheckResponseResult::OK);

  // CS shall have updated config
  BOOST_CHECK_EQUAL(m_cs.getLimit(), 18609);
  BOOST_CHECK_EQUAL(m_cs.shouldAdmit(), true);
  BOOST_CHECK_EQUAL(m_cs.shouldServe(), false);
}

BOOST_AUTO_TEST_CASE(Info)
{
  m_cs.setLimit(2681);
  for (int i = 0; i < 310; ++i) {
    m_cs.insert(*makeData(Name("/Q8H4oi4g").appendSequenceNumber(i)));
  }
  m_cs.enableAdmit(false);
  m_cs.enableServe(true);
  m_fwCnt.nCsHits.set(362);
  m_fwCnt.nCsMisses.set(1493);

  receiveInterest(Interest("/localhost/nfd/cs/info"));
  Block dataset = concatenateResponses();
  dataset.parse();
  BOOST_REQUIRE_EQUAL(dataset.elements_size(), 1);

  ndn::nfd::CsInfo info(*dataset.elements_begin());
  BOOST_CHECK_EQUAL(info.getCapacity(), 2681);
  BOOST_CHECK_EQUAL(info.getEnableAdmit(), false);
  BOOST_CHECK_EQUAL(info.getEnableServe(), true);
  BOOST_CHECK_EQUAL(info.getNEntries(), 310);
  BOOST_CHECK_EQUAL(info.getNHits(), 362);
  BOOST_CHECK_EQUAL(info.getNMisses(), 1493);
}

BOOST_AUTO_TEST_SUITE_END() // TestCsManager
BOOST_AUTO_TEST_SUITE_END() // Mgmt

} // namespace tests
} // namespace nfd
