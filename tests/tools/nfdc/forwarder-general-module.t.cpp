/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "nfdc/forwarder-general-module.hpp"

#include "status-fixture.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestForwarderGeneralModule, StatusFixture<ForwarderGeneralModule>)

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <generalStatus>
    <version>0.4.1-1-g704430c</version>
    <startTime>2016-06-24T15:13:46.856000</startTime>
    <currentTime>2016-07-17T17:55:54.109000</currentTime>
    <uptime>PT1996927S</uptime>
    <nNameTreeEntries>668</nNameTreeEntries>
    <nFibEntries>70</nFibEntries>
    <nPitEntries>7</nPitEntries>
    <nMeasurementsEntries>1</nMeasurementsEntries>
    <nCsEntries>65536</nCsEntries>
    <packetCounters>
      <incomingPackets>
        <nInterests>20699052</nInterests>
        <nData>5598070</nData>
        <nNacks>7230</nNacks>
      </incomingPackets>
      <outgoingPackets>
        <nInterests>36501092</nInterests>
        <nData>5671942</nData>
        <nNacks>26762</nNacks>
      </outgoingPackets>
    </packetCounters>
  </generalStatus>
)XML");

const std::string STATUS_TEXT = std::string(R"TEXT(
General NFD status:
               version=0.4.1-1-g704430c
             startTime=20160624T151346.856000
           currentTime=20160717T175554.109000
                uptime=1996927 seconds
      nNameTreeEntries=668
           nFibEntries=70
           nPitEntries=7
  nMeasurementsEntries=1
            nCsEntries=65536
          nInInterests=20699052
         nOutInterests=36501092
               nInData=5598070
              nOutData=5671942
              nInNacks=7230
             nOutNacks=26762
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(Status)
{
  this->fetchStatus();
  ForwarderStatus payload;
  payload.setNfdVersion("0.4.1-1-g704430c")
         .setStartTimestamp(time::fromUnixTimestamp(time::milliseconds(1466781226856)))
         .setCurrentTimestamp(time::fromUnixTimestamp(time::milliseconds(1468778154109)))
         .setNNameTreeEntries(668)
         .setNFibEntries(70)
         .setNPitEntries(7)
         .setNMeasurementsEntries(1)
         .setNCsEntries(65536)
         .setNInInterests(20699052)
         .setNInData(5598070)
         .setNInNacks(7230)
         .setNOutInterests(36501092)
         .setNOutData(5671942)
         .setNOutNacks(26762);
  this->sendDataset("/localhost/nfd/status/general", payload);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_CASE(StatusNoNfdId)
{
  this->fetchStatus();
  ForwarderStatus payload;
  payload.setNfdVersion("0.4.1-1-g704430c");
  this->sendDataset("/localhost/nfd/status/general", payload);
  BOOST_CHECK_NO_THROW(this->prepareStatusOutput());
}

BOOST_AUTO_TEST_SUITE_END() // TestForwarderGeneralModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
