/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "module-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

BOOST_AUTO_TEST_SUITE(Nfdc)
BOOST_FIXTURE_TEST_SUITE(TestFaceModule, ModuleFixture<FaceModule>)

const std::string STATUS_XML = stripXmlSpaces(R"XML(
  <faces>
    <face>
      <faceId>134</faceId>
      <remoteUri>udp4://233.252.0.4:6363</remoteUri>
      <localUri>udp4://192.0.2.1:6363</localUri>
      <faceScope>non-local</faceScope>
      <facePersistency>permanent</facePersistency>
      <linkType>multi-access</linkType>
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
    " non-local permanent multi-access\n"
  "  faceid=745 remote=fd://75 local=unix:///var/run/nfd.sock"
    " counters={in={18998i 26701d 147n 4672308B} out={34779i 17028d 1176n 8957187B}}"
    " local on-demand point-to-point\n";

BOOST_AUTO_TEST_CASE(Status)
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
