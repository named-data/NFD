/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#include "nfdc/face-module.hpp"
#include <ndn-cxx/mgmt/nfd/face-query-filter.hpp>

#include "execute-command-fixture.hpp"
#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using ndn::nfd::FaceQueryFilter;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_AUTO_TEST_SUITE(TestFaceModule)

BOOST_FIXTURE_TEST_SUITE(ShowCommand, ExecuteCommandFixture)

const std::string NORMAL_OUTPUT = std::string(R"TEXT(
  faceid=256
  remote=udp4://84.67.35.111:6363
   local=udp4://79.91.49.215:6363
counters={in={28975i 28232d 212n 13307258B} out={19525i 30993d 1038n 6231946B}}
   flags={non-local on-demand point-to-point}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK_EQUAL(interest.getName().size(), 5);
    FaceQueryFilter filter(interest.getName().at(4).blockFromValue());
    BOOST_CHECK_EQUAL(filter, FaceQueryFilter().setFaceId(256));

    FaceStatus payload;
    payload.setFaceId(256)
           .setRemoteUri("udp4://84.67.35.111:6363")
           .setLocalUri("udp4://79.91.49.215:6363")
           .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
           .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND)
           .setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT)
           .setNInInterests(28975)
           .setNInDatas(28232)
           .setNInNacks(212)
           .setNOutInterests(19525)
           .setNOutDatas(30993)
           .setNOutNacks(1038)
           .setNInBytes(13307258)
           .setNOutBytes(6231946);

    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NORMAL_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NotFound)
{
  this->processInterest = [this] (const Interest& interest) {
    this->sendEmptyDataset(interest.getName());
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 3);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Face not found\n"));
}

BOOST_AUTO_TEST_CASE(Error)
{
  this->processInterest = nullptr; // no response

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when querying face: Timeout\n"));
}

BOOST_AUTO_TEST_SUITE_END() // ShowCommand

BOOST_FIXTURE_TEST_SUITE(CreateCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(Normal)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/faces/create");
    BOOST_REQUIRE(req.hasUri());
    BOOST_CHECK_EQUAL(req.getUri(), "udp4://159.242.33.78:6363");
    BOOST_REQUIRE(req.hasFacePersistency());
    BOOST_CHECK_EQUAL(req.getFacePersistency(), FacePersistency::FACE_PERSISTENCY_PERSISTENT);

    ControlParameters resp;
    resp.setFaceId(2130)
        .setUri("udp4://159.242.33.78:6363")
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
    this->succeedCommand(resp);
  };

  this->execute("face create udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-created id=2130 remote=udp4://159.242.33.78:6363 persistency=persistent\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(Error)
{
  this->processInterest = nullptr; // no response

  this->execute("face create udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when creating face: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // CreateCommand

BOOST_FIXTURE_TEST_SUITE(DestroyCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(NormalByFaceId)
{
  this->processInterest = [this] (const Interest& interest) {
    if (Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName())) {
      BOOST_CHECK_EQUAL(interest.getName().size(), 5);
      FaceQueryFilter filter(interest.getName().at(4).blockFromValue());
      BOOST_CHECK_EQUAL(filter, FaceQueryFilter().setFaceId(10156));

      FaceStatus faceStatus;
      faceStatus.setFaceId(10156)
                .setLocalUri("tcp4://151.26.163.27:22967")
                .setRemoteUri("tcp4://198.57.27.40:6363")
                .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      this->sendDataset(interest.getName(), faceStatus);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/faces/destroy");
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 10156);

    ControlParameters resp;
    resp.setFaceId(10156);
    this->succeedCommand(resp);
  };

  this->execute("face destroy 10156");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-destroyed id=10156 local=tcp4://151.26.163.27:22967 "
                           "remote=tcp4://198.57.27.40:6363 persistency=persistent\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NormalByFaceUri)
{
  this->processInterest = [this] (const Interest& interest) {
    if (Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName())) {
      BOOST_CHECK_EQUAL(interest.getName().size(), 5);
      FaceQueryFilter filter(interest.getName().at(4).blockFromValue());
      BOOST_CHECK_EQUAL(filter, FaceQueryFilter().setRemoteUri("tcp4://32.121.182.82:6363"));

      FaceStatus faceStatus;
      faceStatus.setFaceId(2249)
                .setLocalUri("tcp4://30.99.87.98:31414")
                .setRemoteUri("tcp4://32.121.182.82:6363")
                .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      this->sendDataset(interest.getName(), faceStatus);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/faces/destroy");
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 2249);

    ControlParameters resp;
    resp.setFaceId(2249);
    this->succeedCommand(resp);
  };

  this->execute("face destroy tcp://32.121.182.82");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-destroyed id=2249 local=tcp4://30.99.87.98:31414 "
                           "remote=tcp4://32.121.182.82:6363 persistency=persistent\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(FaceNotExist)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
    this->sendEmptyDataset(interest.getName());
  };

  this->execute("face destroy 23728");
  BOOST_CHECK_EQUAL(exitCode, 3);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Face not found\n"));
}

BOOST_AUTO_TEST_CASE(Ambiguous)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));

    FaceStatus faceStatus1, faceStatus2;
    faceStatus1.setFaceId(6720)
               .setLocalUri("udp4://202.83.168.28:56363")
               .setRemoteUri("udp4://225.131.75.231:56363")
               .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERMANENT);
    faceStatus2.setFaceId(31066)
               .setLocalUri("udp4://25.90.26.32:56363")
               .setRemoteUri("udp4://225.131.75.231:56363")
               .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERMANENT);
    this->sendDataset(interest.getName(), faceStatus1, faceStatus2);
  };

  this->execute("face destroy udp4://225.131.75.231:56363");
  BOOST_CHECK_EQUAL(exitCode, 5);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Multiple faces match specified remote FaceUri. "
                           "Re-run the command with a FaceId: "
                           "6720 (local=udp4://202.83.168.28:56363), "
                           "31066 (local=udp4://25.90.26.32:56363)\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCanonization)
{
  this->execute("face destroy udp6://32.38.164.64:10445");
  BOOST_CHECK_EQUAL(exitCode, 4);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error during remote FaceUri canonization: "
                           "No endpoints match the specified address selector\n"));
}

BOOST_AUTO_TEST_CASE(ErrorDataset)
{
  this->processInterest = nullptr; // no response to dataset or command

  this->execute("face destroy udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when querying face: Timeout\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCommand)
{
  this->processInterest = [this] (const Interest& interest) {
    if (Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName())) {
      FaceStatus faceStatus;
      faceStatus.setFaceId(17757)
                .setLocalUri("tcp4://27.65.24.30:19187")
                .setRemoteUri("tcp4://70.47.27.77:6363")
                .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      this->sendDataset(interest.getName(), faceStatus);
      return;
    }

    MOCK_NFD_MGMT_REQUIRE_LAST_COMMAND_IS("/localhost/nfd/faces/destroy");
    // no response to command
  };

  this->execute("face destroy 17757");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when destroying face: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // DestroyCommand

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <faces>
    <face>
      <faceId>134</faceId>
      <remoteUri>udp4://233.252.0.4:6363</remoteUri>
      <localUri>udp4://192.0.2.1:6363</localUri>
      <faceScope>non-local</faceScope>
      <facePersistency>permanent</facePersistency>
      <linkType>multi-access</linkType>
      <flags/>
      <packetCounters>
        <incomingPackets>
          <nInterests>22562</nInterests>
          <nDatas>22031</nDatas>
          <nNacks>63</nNacks>
        </incomingPackets>
        <outgoingPackets>
          <nInterests>30121</nInterests>
          <nDatas>20940</nDatas>
          <nNacks>1218</nNacks>
        </outgoingPackets>
      </packetCounters>
      <byteCounters>
        <incomingBytes>2522915</incomingBytes>
        <outgoingBytes>1353592</outgoingBytes>
      </byteCounters>
    </face>
    <face>
      <faceId>745</faceId>
      <remoteUri>fd://75</remoteUri>
      <localUri>unix:///var/run/nfd.sock</localUri>
      <faceScope>local</faceScope>
      <facePersistency>on-demand</facePersistency>
      <linkType>point-to-point</linkType>
      <flags>
        <localFieldsEnabled/>
      </flags>
      <packetCounters>
        <incomingPackets>
          <nInterests>18998</nInterests>
          <nDatas>26701</nDatas>
          <nNacks>147</nNacks>
        </incomingPackets>
        <outgoingPackets>
          <nInterests>34779</nInterests>
          <nDatas>17028</nDatas>
          <nNacks>1176</nNacks>
        </outgoingPackets>
      </packetCounters>
      <byteCounters>
        <incomingBytes>4672308</incomingBytes>
        <outgoingBytes>8957187</outgoingBytes>
      </byteCounters>
    </face>
  </faces>
)XML");

const std::string STATUS_TEXT =
  "Faces:\n"
  "  faceid=134 remote=udp4://233.252.0.4:6363 local=udp4://192.0.2.1:6363"
    " counters={in={22562i 22031d 63n 2522915B} out={30121i 20940d 1218n 1353592B}}"
    " flags={non-local permanent multi-access}\n"
  "  faceid=745 remote=fd://75 local=unix:///var/run/nfd.sock"
    " counters={in={18998i 26701d 147n 4672308B} out={34779i 17028d 1176n 8957187B}}"
    " flags={local on-demand point-to-point local-fields}\n";

BOOST_FIXTURE_TEST_CASE(Status, StatusFixture<FaceModule>)
{
  this->fetchStatus();
  FaceStatus payload1;
  payload1.setFaceId(134)
          .setRemoteUri("udp4://233.252.0.4:6363")
          .setLocalUri("udp4://192.0.2.1:6363")
          .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
          .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT)
          .setLinkType(ndn::nfd::LINK_TYPE_MULTI_ACCESS)
          .setNInInterests(22562)
          .setNInDatas(22031)
          .setNInNacks(63)
          .setNOutInterests(30121)
          .setNOutDatas(20940)
          .setNOutNacks(1218)
          .setNInBytes(2522915)
          .setNOutBytes(1353592);
  FaceStatus payload2;
  payload2.setFaceId(745)
          .setRemoteUri("fd://75")
          .setLocalUri("unix:///var/run/nfd.sock")
          .setFaceScope(ndn::nfd::FACE_SCOPE_LOCAL)
          .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_ON_DEMAND)
          .setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT)
          .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true)
          .setNInInterests(18998)
          .setNInDatas(26701)
          .setNInNacks(147)
          .setNOutInterests(34779)
          .setNOutDatas(17028)
          .setNOutNacks(1176)
          .setNInBytes(4672308)
          .setNOutBytes(8957187);
  this->sendDataset("/localhost/nfd/faces/list", payload1, payload2);
  this->prepareStatusOutput();

  BOOST_CHECK(statusXml.is_equal(STATUS_XML));
  BOOST_CHECK(statusText.is_equal(STATUS_TEXT));
}

BOOST_AUTO_TEST_SUITE_END() // TestFaceModule
BOOST_AUTO_TEST_SUITE_END() // Nfdc

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd
