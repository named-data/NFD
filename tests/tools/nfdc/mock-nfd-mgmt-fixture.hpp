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

#ifndef NFD_TESTS_TOOLS_NFDC_MOCK_NFD_MGMT_FIXTURE_HPP
#define NFD_TESTS_TOOLS_NFDC_MOCK_NFD_MGMT_FIXTURE_HPP

#include "../mock-nfd-mgmt-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;

/** \brief fixture to emulate NFD management
 */
class MockNfdMgmtFixture : public nfd::tools::tests::MockNfdMgmtFixture
{
protected:
  /** \brief respond to specific FaceQuery requests
   *  \retval true the Interest matches one of the defined patterns and is responded
   *  \retval false the Interest is not responded
   */
  bool
  respondFaceQuery(const Interest& interest)
  {
    using ndn::nfd::FacePersistency;
    using ndn::nfd::FaceQueryFilter;
    using ndn::nfd::FaceStatus;

    if (!Name("/localhost/nfd/faces/query").isPrefixOf(interest.getName())) {
      return false;
    }
    BOOST_CHECK_EQUAL(interest.getName().size(), 5);
    FaceQueryFilter filter(interest.getName().at(4).blockFromValue());

    if (filter == FaceQueryFilter().setFaceId(10156)) {
      FaceStatus faceStatus;
      faceStatus.setFaceId(10156)
                .setLocalUri("tcp4://151.26.163.27:22967")
                .setRemoteUri("tcp4://198.57.27.40:6363")
                .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      this->sendDataset(interest.getName(), faceStatus);
      return true;
    }

    if (filter == FaceQueryFilter().setRemoteUri("tcp4://32.121.182.82:6363")) {
      FaceStatus faceStatus;
      faceStatus.setFaceId(2249)
                .setLocalUri("tcp4://30.99.87.98:31414")
                .setRemoteUri("tcp4://32.121.182.82:6363")
                .setFacePersistency(FacePersistency::FACE_PERSISTENCY_PERSISTENT);
      this->sendDataset(interest.getName(), faceStatus);
      return true;
    }

    if (filter == FaceQueryFilter().setFaceId(23728)) {
      this->sendEmptyDataset(interest.getName());
      return true;
    }

    if (filter == FaceQueryFilter().setRemoteUri("udp4://225.131.75.231:56363")) {
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
      return true;
    }

    return false;
  }
};

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TESTS_TOOLS_NFDC_MOCK_NFD_MGMT_FIXTURE_HPP
