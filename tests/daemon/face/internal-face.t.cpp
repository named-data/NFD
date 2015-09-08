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

#include "face/internal-face.hpp"
#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

class InternalFaceFixture : protected BaseFixture
{
public:
  InternalFaceFixture()
  {
    m_face.onReceiveInterest.connect([this] (const Interest& interest) {
        inInterests.push_back(interest);
      });
    m_face.onSendInterest.connect([this] (const Interest& interest) {
        outInterests.push_back(interest);
      });
    m_face.onReceiveData.connect([this] (const Data& data) {
        inData.push_back(data);
      });
    m_face.onSendData.connect([this] (const Data& data) {
        outData.push_back(data);
      });
  }

protected:
  InternalFace m_face;
  std::vector<Interest> inInterests;
  std::vector<Interest> outInterests;
  std::vector<Data> inData;
  std::vector<Data> outData;
};

BOOST_FIXTURE_TEST_SUITE(FaceInternalFace, InternalFaceFixture)

BOOST_AUTO_TEST_CASE(SendInterest)
{
  BOOST_CHECK(outInterests.empty());

  auto interest = makeInterest("/test/send/interest");
  Block expectedBlock = interest->wireEncode(); // assign a value to nonce
  m_face.sendInterest(*interest);

  BOOST_CHECK_EQUAL(outInterests.size(), 1);
  BOOST_CHECK(outInterests[0].wireEncode() == expectedBlock);
}

BOOST_AUTO_TEST_CASE(SendData)
{
  BOOST_CHECK(outData.empty());

  auto data = makeData("/test/send/data");
  m_face.sendData(*data);

  BOOST_CHECK_EQUAL(outData.size(), 1);
  BOOST_CHECK(outData[0].wireEncode() == data->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveInterest)
{
  BOOST_CHECK(inInterests.empty());

  auto interest = makeInterest("/test/receive/interest");
  Block expectedBlock = interest->wireEncode(); // assign a value to nonce
  m_face.receiveInterest(*interest);

  BOOST_CHECK_EQUAL(inInterests.size(), 1);
  BOOST_CHECK(inInterests[0].wireEncode() == expectedBlock);
}

BOOST_AUTO_TEST_CASE(ReceiveData)
{
  BOOST_CHECK(inData.empty());

  auto data = makeData("/test/send/data");
  m_face.receiveData(*data);

  BOOST_CHECK_EQUAL(inData.size(), 1);
  BOOST_CHECK(inData[0].wireEncode() == data->wireEncode());
}

BOOST_AUTO_TEST_CASE(ReceiveBlock)
{
  BOOST_CHECK(inInterests.empty());
  BOOST_CHECK(inData.empty());

  Block interestBlock = makeInterest("test/receive/interest")->wireEncode();
  m_face.receive(interestBlock);
  BOOST_CHECK_EQUAL(inInterests.size(), 1);
  BOOST_CHECK(inInterests[0].wireEncode() == interestBlock);

  Block dataBlock = makeData("test/receive/data")->wireEncode();
  m_face.receive(dataBlock);
  BOOST_CHECK_EQUAL(inData.size(), 1);
  BOOST_CHECK(inData[0].wireEncode() == dataBlock);
}

BOOST_AUTO_TEST_CASE(ExtractPacketFromBlock)
{
  {
    Block block = makeInterest("/test/interest")->wireEncode();
    const Block& payload = ndn::nfd::LocalControlHeader::getPayload(block);
    auto interest = m_face.extractPacketFromBlock<Interest>(block, payload);
    BOOST_CHECK(interest->wireEncode() == block);
  }

  {
    Block block = makeData("/test/data")->wireEncode();
    const Block& payload = ndn::nfd::LocalControlHeader::getPayload(block);
    auto data = m_face.extractPacketFromBlock<Data>(block, payload);
    BOOST_CHECK(data->wireEncode() == block);
  }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
