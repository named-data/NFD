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

#include "nfdc/face-module.hpp"

#include "execute-command-fixture.hpp"
#include "status-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using ndn::nfd::FaceQueryFilter;

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_AUTO_TEST_SUITE(TestFaceModule)

BOOST_FIXTURE_TEST_SUITE(ListCommand, ExecuteCommandFixture)

const std::string NONQUERY_OUTPUT =
  "faceid=134 remote=udp4://233.252.0.4:6363 local=udp4://192.0.2.1:6363"
    " congestion={base-marking-interval=12345ms default-threshold=54321B}"
    " counters={in={22562i 22031d 63n 2522915B} out={30121i 20940d 1218n 1353592B}}"
    " flags={non-local permanent multi-access}\n"
  "faceid=745 remote=fd://75 local=unix:///var/run/nfd.sock"
    " congestion={base-marking-interval=100ms default-threshold=65536B}"
    " counters={in={18998i 26701d 147n 4672308B} out={34779i 17028d 1176n 8957187B}}"
    " flags={local on-demand point-to-point local-fields lp-reliability congestion-marking}\n";

BOOST_AUTO_TEST_CASE(NormalNonQuery)
{
  this->processInterest = [this] (const Interest& interest) {
    FaceStatus payload1;
    payload1.setFaceId(134)
            .setRemoteUri("udp4://233.252.0.4:6363")
            .setLocalUri("udp4://192.0.2.1:6363")
            .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
            .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERMANENT)
            .setLinkType(ndn::nfd::LINK_TYPE_MULTI_ACCESS)
            .setBaseCongestionMarkingInterval(12345_ms)
            .setDefaultCongestionThreshold(54321)
            .setNInInterests(22562)
            .setNInData(22031)
            .setNInNacks(63)
            .setNOutInterests(30121)
            .setNOutData(20940)
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
            .setBaseCongestionMarkingInterval(100_ms)
            .setDefaultCongestionThreshold(65536)
            .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true)
            .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true)
            .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true)
            .setNInInterests(18998)
            .setNInData(26701)
            .setNInNacks(147)
            .setNOutInterests(34779)
            .setNOutData(17028)
            .setNOutNacks(1176)
            .setNInBytes(4672308)
            .setNOutBytes(8957187);
    this->sendDataset("/localhost/nfd/faces/list", payload1, payload2);
  };

  this->execute("face list");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NONQUERY_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

const std::string QUERY_OUTPUT =
  "faceid=177 remote=tcp4://53.239.9.114:6363 local=tcp4://164.0.31.106:20396"
    " congestion={base-marking-interval=555ms default-threshold=10000B}"
    " counters={in={2325i 1110d 79n 4716834B} out={2278i 485d 841n 308108B}}"
    " flags={non-local persistent point-to-point}\n";

BOOST_AUTO_TEST_CASE(NormalQuery)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
    BOOST_CHECK_EQUAL(interest.getName().size(), 5);
    FaceQueryFilter filter(interest.getName().at(4).blockFromValue());
    FaceQueryFilter expectedFilter;
    expectedFilter.setRemoteUri("tcp4://53.239.9.114:6363")
                  .setLocalUri("tcp4://164.0.31.106:20396")
                  .setUriScheme("tcp4");
    BOOST_CHECK_EQUAL(filter, expectedFilter);

    FaceStatus payload;
    payload.setFaceId(177)
           .setRemoteUri("tcp4://53.239.9.114:6363")
           .setLocalUri("tcp4://164.0.31.106:20396")
           .setFaceScope(ndn::nfd::FACE_SCOPE_NON_LOCAL)
           .setFacePersistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
           .setLinkType(ndn::nfd::LINK_TYPE_POINT_TO_POINT)
           .setBaseCongestionMarkingInterval(555_ms)
           .setDefaultCongestionThreshold(10000)
           .setNInInterests(2325)
           .setNInData(1110)
           .setNInNacks(79)
           .setNOutInterests(2278)
           .setNOutData(485)
           .setNOutNacks(841)
           .setNInBytes(4716834)
           .setNOutBytes(308108);
    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face list tcp://53.239.9.114 scheme tcp4 local tcp://164.0.31.106:20396");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(QUERY_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NotFound)
{
  this->processInterest = [this] (const Interest& interest) {
    this->sendEmptyDataset(interest.getName());
  };

  this->execute("face list scheme udp6");
  BOOST_CHECK_EQUAL(exitCode, 3);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Face not found\n"));
}

BOOST_AUTO_TEST_CASE(Error)
{
  this->processInterest = nullptr; // no response

  this->execute("face list local udp4://31.67.17.2:6363");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when querying face: Timeout\n"));
}

BOOST_AUTO_TEST_SUITE_END() // ListCommand

BOOST_FIXTURE_TEST_SUITE(ShowCommand, ExecuteCommandFixture)

const std::string NORMAL_ALL_CONGESTION_OUTPUT = std::string(R"TEXT(
    faceid=256
    remote=udp4://84.67.35.111:6363
     local=udp4://79.91.49.215:6363
congestion={base-marking-interval=123ms default-threshold=10000B}
  counters={in={28975i 28232d 212n 13307258B} out={19525i 30993d 1038n 6231946B}}
     flags={non-local on-demand point-to-point}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(NormalAllCongestionParams)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
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
           .setBaseCongestionMarkingInterval(123_ms)
           .setDefaultCongestionThreshold(10000)
           .setNInInterests(28975)
           .setNInData(28232)
           .setNInNacks(212)
           .setNOutInterests(19525)
           .setNOutData(30993)
           .setNOutNacks(1038)
           .setNInBytes(13307258)
           .setNOutBytes(6231946);

    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NORMAL_ALL_CONGESTION_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

const std::string NORMAL_INTERVAL_CONGESTION_OUTPUT = std::string(R"TEXT(
    faceid=256
    remote=udp4://84.67.35.111:6363
     local=udp4://79.91.49.215:6363
congestion={base-marking-interval=123ms}
  counters={in={28975i 28232d 212n 13307258B} out={19525i 30993d 1038n 6231946B}}
     flags={non-local on-demand point-to-point}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(NormalIntervalCongestionParams)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
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
           .setBaseCongestionMarkingInterval(123_ms)
           .setNInInterests(28975)
           .setNInData(28232)
           .setNInNacks(212)
           .setNOutInterests(19525)
           .setNOutData(30993)
           .setNOutNacks(1038)
           .setNInBytes(13307258)
           .setNOutBytes(6231946);

    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NORMAL_INTERVAL_CONGESTION_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

const std::string NORMAL_THRESHOLD_CONGESTION_OUTPUT = std::string(R"TEXT(
    faceid=256
    remote=udp4://84.67.35.111:6363
     local=udp4://79.91.49.215:6363
congestion={default-threshold=10000B}
  counters={in={28975i 28232d 212n 13307258B} out={19525i 30993d 1038n 6231946B}}
     flags={non-local on-demand point-to-point}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(NormalThresholdCongestionParams)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
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
           .setDefaultCongestionThreshold(10000)
           .setNInInterests(28975)
           .setNInData(28232)
           .setNInNacks(212)
           .setNOutInterests(19525)
           .setNOutData(30993)
           .setNOutNacks(1038)
           .setNInBytes(13307258)
           .setNOutBytes(6231946);

    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NORMAL_THRESHOLD_CONGESTION_OUTPUT));
  BOOST_CHECK(err.is_empty());
}

const std::string NORMAL_NO_CONGESTION_OUTPUT = std::string(R"TEXT(
    faceid=256
    remote=udp4://84.67.35.111:6363
     local=udp4://79.91.49.215:6363
  counters={in={28975i 28232d 212n 13307258B} out={19525i 30993d 1038n 6231946B}}
     flags={non-local on-demand point-to-point}
)TEXT").substr(1);

BOOST_AUTO_TEST_CASE(NormalNoCongestionParams)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName()));
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
           .setNInData(28232)
           .setNInNacks(212)
           .setNOutInterests(19525)
           .setNOutData(30993)
           .setNOutNacks(1038)
           .setNInBytes(13307258)
           .setNOutBytes(6231946);

    this->sendDataset(interest.getName(), payload);
  };

  this->execute("face show 256");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal(NORMAL_NO_CONGESTION_OUTPUT));
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

class ExecuteFaceCreateCommandFixture : public ExecuteCommandFixture
{
protected:
  void
  respond409(const Interest& interest, FacePersistency persistency,
             bool enableLpReliability = false,
             bool enableCongestionMarking = false)
  {
    MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/create");
    ControlParameters body;
    body.setFaceId(1172)
        .setUri("udp4://100.77.30.65:6363")
        .setLocalUri("udp4://68.62.26.57:24087")
        .setFacePersistency(persistency)
        .setFlags(0);
    if (enableLpReliability) {
      body.setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true, false);
    }
    if (enableCongestionMarking) {
      body.setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true, false);
    }
    this->failCommand(interest, 409, "conflict-409", body);
  }
};

BOOST_FIXTURE_TEST_SUITE(CreateCommand, ExecuteFaceCreateCommandFixture)

BOOST_AUTO_TEST_CASE(Creating)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/create");
    BOOST_REQUIRE(req.hasUri());
    BOOST_CHECK_EQUAL(req.getUri(), "udp4://159.242.33.78:6363");
    BOOST_CHECK(!req.hasLocalUri());
    BOOST_REQUIRE(req.hasFacePersistency());
    BOOST_CHECK_EQUAL(req.getFacePersistency(), FacePersistency::FACE_PERSISTENCY_PERSISTENT);

    ControlParameters resp;
    resp.setFaceId(2130)
        .setUri("udp4://159.242.33.78:6363")
        .setLocalUri("udp4://179.63.153.45:28835")
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setFlags(0);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-created id=2130 local=udp4://179.63.153.45:28835 "
                           "remote=udp4://159.242.33.78:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(CreatingWithParams)
{
  this->processInterest = [this] (const Interest& interest) {
    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/create");
    BOOST_REQUIRE(req.hasUri());
    BOOST_CHECK_EQUAL(req.getUri(), "udp4://22.91.89.51:19903");
    BOOST_REQUIRE(req.hasLocalUri());
    BOOST_CHECK_EQUAL(req.getLocalUri(), "udp4://98.68.23.71:6363");
    BOOST_REQUIRE(req.hasFacePersistency());
    BOOST_CHECK_EQUAL(req.getFacePersistency(), FacePersistency::FACE_PERSISTENCY_PERMANENT);
    BOOST_CHECK(req.hasFlags());
    BOOST_CHECK(req.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));

    ControlParameters resp;
    resp.setFaceId(301)
        .setUri("udp4://22.91.89.51:19903")
        .setLocalUri("udp4://98.68.23.71:6363")
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERMANENT)
        .setBaseCongestionMarkingInterval(100_ms)
        .setDefaultCongestionThreshold(65536)
        .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true, false)
        .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://22.91.89.51:19903 permanent local udp://98.68.23.71 reliability on "
                "congestion-marking on congestion-marking-interval 100 "
                "default-congestion-threshold 65536");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-created id=301 local=udp4://98.68.23.71:6363 "
                           "remote=udp4://22.91.89.51:19903 persistency=permanent "
                           "reliability=on congestion-marking=on "
                           "congestion-marking-interval=100ms default-congestion-threshold=65536B\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(UpgradingPersistency)
{
  bool hasUpdateCommand = false;
  this->processInterest = [this, &hasUpdateCommand] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_ON_DEMAND);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    hasUpdateCommand = true;
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 1172);
    BOOST_REQUIRE(req.hasFacePersistency());
    BOOST_CHECK_EQUAL(req.getFacePersistency(), FacePersistency::FACE_PERSISTENCY_PERSISTENT);
    BOOST_CHECK(!req.hasFlags());

    ControlParameters resp;
    resp.setFaceId(1172)
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setFlags(0);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://100.77.30.65");
  BOOST_CHECK(hasUpdateCommand);
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-updated id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NotDowngradingPersistency)
{
  this->processInterest = [this] (const Interest& interest) {
    this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERMANENT);
    // no command other than faces/create is expected
  };

  this->execute("face create udp://100.77.30.65");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-exists id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=permanent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(SamePersistency)
{
  this->processInterest = [this] (const Interest& interest) {
    this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERSISTENT);
    // no command other than faces/create is expected
  };

  this->execute("face create udp://100.77.30.65");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-exists id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(EnablingReliability)
{
  bool hasUpdateCommand = false;
  this->processInterest = [this, &hasUpdateCommand] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    hasUpdateCommand = true;
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 1172);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
    BOOST_CHECK(req.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));

    ControlParameters resp;
    resp.setFaceId(1172)
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://100.77.30.65 reliability on");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-updated id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=on congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(DisablingReliability)
{
  bool hasUpdateCommand = false;
  this->processInterest = [this, &hasUpdateCommand] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERSISTENT, true);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    hasUpdateCommand = true;
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 1172);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));
    BOOST_CHECK(!req.getFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED));

    ControlParameters resp;
    resp.setFaceId(1172)
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, false, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://100.77.30.65 reliability off");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-updated id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(EnablingCongestionMarking)
{
  bool hasUpdateCommand = false;
  this->processInterest = [this, &hasUpdateCommand] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    hasUpdateCommand = true;
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 1172);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
    BOOST_CHECK(req.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
    BOOST_CHECK(!req.hasBaseCongestionMarkingInterval());
    BOOST_CHECK(!req.hasDefaultCongestionThreshold());

    ControlParameters resp;
    resp.setFaceId(1172)
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setBaseCongestionMarkingInterval(100_ms)
        .setDefaultCongestionThreshold(65536)
        .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://100.77.30.65 congestion-marking on");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-updated id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=off congestion-marking=on "
                           "congestion-marking-interval=100ms default-congestion-threshold=65536B\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(DisablingCongestionMarking)
{
  bool hasUpdateCommand = false;
  this->processInterest = [this, &hasUpdateCommand] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_PERSISTENT, false, true);
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    hasUpdateCommand = true;
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 1172);
    BOOST_CHECK(req.hasFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
    BOOST_CHECK(!req.getFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED));
    BOOST_CHECK(!req.hasBaseCongestionMarkingInterval());
    BOOST_CHECK(!req.hasDefaultCongestionThreshold());

    ControlParameters resp;
    resp.setFaceId(1172)
        .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT)
        .setBaseCongestionMarkingInterval(100_ms)
        .setDefaultCongestionThreshold(65536)
        .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, false, false);
    this->succeedCommand(interest, resp);
  };

  this->execute("face create udp://100.77.30.65 congestion-marking off");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-updated id=1172 local=udp4://68.62.26.57:24087 "
                           "remote=udp4://100.77.30.65:6363 persistency=persistent "
                           "reliability=off congestion-marking=off "
                           "congestion-marking-interval=100ms default-congestion-threshold=65536B\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(ErrorCanonizeRemote)
{
  this->execute("face create invalid://");
  BOOST_CHECK_EQUAL(exitCode, 4);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error when canonizing 'invalid://': scheme not supported\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCanonizeLocal)
{
  this->execute("face create udp4://24.37.20.47:6363 local invalid://");
  BOOST_CHECK_EQUAL(exitCode, 4);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error when canonizing 'invalid://': scheme not supported\n"));
}

BOOST_AUTO_TEST_CASE(ErrorCreate)
{
  this->processInterest = nullptr; // no response

  this->execute("face create udp://159.242.33.78");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when creating face: request timed out\n"));
}

BOOST_AUTO_TEST_CASE(ErrorConflict)
{
  // Current NFD will not report a 409-conflict with a different remote FaceUri, but this is
  // allowed by FaceMgmt protocol and nfdc should not attempt to upgrade persistency in this case.

  this->processInterest = [this] (const Interest& interest) {
    // conflict with udp4://100.77.30.65:6363
    this->respond409(interest, FacePersistency::FACE_PERSISTENCY_ON_DEMAND);
  };

  this->execute("face create udp://20.53.73.45");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 409 when creating face: conflict-409\n"));
}

BOOST_AUTO_TEST_CASE(ErrorUpdate)
{
  this->processInterest = [this] (const Interest& interest) {
    if (parseCommand(interest, "/localhost/nfd/faces/create")) {
      this->respond409(interest, FacePersistency::FACE_PERSISTENCY_ON_DEMAND);
      return;
    }

    MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/update");
    // no response to faces/update
  };

  this->execute("face create udp://100.77.30.65");
  BOOST_CHECK_EQUAL(exitCode, 1);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Error 10060 when upgrading face persistency: request timed out\n"));
}

BOOST_AUTO_TEST_SUITE_END() // CreateCommand

BOOST_FIXTURE_TEST_SUITE(DestroyCommand, ExecuteCommandFixture)

BOOST_AUTO_TEST_CASE(NormalByFaceId)
{
  this->processInterest = [this] (const Interest& interest) {
    if (this->respondFaceQuery(interest)) {
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/destroy");
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 10156);

    ControlParameters resp;
    resp.setFaceId(10156);
    this->succeedCommand(interest, resp);
  };

  this->execute("face destroy 10156");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-destroyed id=10156 local=tcp4://151.26.163.27:22967 "
                           "remote=tcp4://198.57.27.40:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(NormalByFaceUri)
{
  this->processInterest = [this] (const Interest& interest) {
    if (this->respondFaceQuery(interest)) {
      return;
    }

    ControlParameters req = MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/destroy");
    BOOST_REQUIRE(req.hasFaceId());
    BOOST_CHECK_EQUAL(req.getFaceId(), 2249);

    ControlParameters resp;
    resp.setFaceId(2249);
    this->succeedCommand(interest, resp);
  };

  this->execute("face destroy tcp://32.121.182.82");
  BOOST_CHECK_EQUAL(exitCode, 0);
  BOOST_CHECK(out.is_equal("face-destroyed id=2249 local=tcp4://30.99.87.98:31414 "
                           "remote=tcp4://32.121.182.82:6363 persistency=persistent "
                           "reliability=off congestion-marking=off\n"));
  BOOST_CHECK(err.is_empty());
}

BOOST_AUTO_TEST_CASE(FaceNotExist)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondFaceQuery(interest));
  };

  this->execute("face destroy 23728");
  BOOST_CHECK_EQUAL(exitCode, 3);
  BOOST_CHECK(out.is_empty());
  BOOST_CHECK(err.is_equal("Face not found\n"));
}

BOOST_AUTO_TEST_CASE(Ambiguous)
{
  this->processInterest = [this] (const Interest& interest) {
    BOOST_CHECK(this->respondFaceQuery(interest));
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
                           "IPv4/v6 mismatch\n"));
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
    if (this->respondFaceQuery(interest)) {
      return;
    }

    MOCK_NFD_MGMT_REQUIRE_COMMAND_IS("/localhost/nfd/faces/destroy");
    // no response to command
  };

  this->execute("face destroy 10156");
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
      <congestion/>
      <flags/>
      <packetCounters>
        <incomingPackets>
          <nInterests>22562</nInterests>
          <nData>22031</nData>
          <nNacks>63</nNacks>
        </incomingPackets>
        <outgoingPackets>
          <nInterests>30121</nInterests>
          <nData>20940</nData>
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
      <congestion>
        <baseMarkingInterval>PT0.100S</baseMarkingInterval>
        <defaultThreshold>65536</defaultThreshold>
      </congestion>
      <flags>
        <localFieldsEnabled/>
        <lpReliabilityEnabled/>
        <congestionMarkingEnabled/>
      </flags>
      <packetCounters>
        <incomingPackets>
          <nInterests>18998</nInterests>
          <nData>26701</nData>
          <nNacks>147</nNacks>
        </incomingPackets>
        <outgoingPackets>
          <nInterests>34779</nInterests>
          <nData>17028</nData>
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
    " congestion={base-marking-interval=100ms default-threshold=65536B}"
    " counters={in={18998i 26701d 147n 4672308B} out={34779i 17028d 1176n 8957187B}}"
    " flags={local on-demand point-to-point local-fields lp-reliability congestion-marking}\n";

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
          .setNInData(22031)
          .setNInNacks(63)
          .setNOutInterests(30121)
          .setNOutData(20940)
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
          .setBaseCongestionMarkingInterval(100_ms)
          .setDefaultCongestionThreshold(65536)
          .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true)
          .setFlagBit(ndn::nfd::BIT_LP_RELIABILITY_ENABLED, true)
          .setFlagBit(ndn::nfd::BIT_CONGESTION_MARKING_ENABLED, true)
          .setNInInterests(18998)
          .setNInData(26701)
          .setNInNacks(147)
          .setNOutInterests(34779)
          .setNOutData(17028)
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
