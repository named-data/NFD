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

#include "face/face-counters.hpp"
#include "dummy-face.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FaceFaceCounters, BaseFixture)

BOOST_AUTO_TEST_CASE(PacketCnt)
{
  PacketCounter counter;

  uint64_t observation = counter;//implicit convertible
  BOOST_CHECK_EQUAL(observation, 0);

  ++counter;
  BOOST_CHECK_EQUAL(static_cast<int>(counter), 1);
  ++counter;
  ++counter;
  BOOST_CHECK_EQUAL(static_cast<int>(counter), 3);
}

BOOST_AUTO_TEST_CASE(ByteCnt)
{
  ByteCounter counter;

  uint64_t observation = counter;//implicit convertible
  BOOST_CHECK_EQUAL(observation, 0);

  counter += 20;
  BOOST_CHECK_EQUAL(static_cast<int>(counter), 20);
  counter += 80;
  counter += 90;
  BOOST_CHECK_EQUAL(static_cast<int>(counter), 190);
}

BOOST_AUTO_TEST_CASE(Counters)
{
  DummyFace face;
  const FaceCounters& counters = face.getCounters();
  BOOST_CHECK_EQUAL(counters.getNInInterests() , 0);
  BOOST_CHECK_EQUAL(counters.getNInDatas()     , 0);
  BOOST_CHECK_EQUAL(counters.getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(counters.getNOutDatas()    , 0);
  BOOST_CHECK_EQUAL(counters.getNInBytes()     , 0);
  BOOST_CHECK_EQUAL(counters.getNOutBytes()    , 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
