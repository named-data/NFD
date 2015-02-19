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

#include "fw/rtt-estimator.hpp"

#include "tests/test-common.hpp"

namespace nfd {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(FwRttEstimator, BaseFixture)

static inline double
computeRtoAsFloatSeconds(RttEstimator& rtt)
{
  typedef time::duration<double, time::seconds::period> FloatSeconds;

  RttEstimator::Duration rto = rtt.computeRto();
  return time::duration_cast<FloatSeconds>(rto).count();
}

BOOST_AUTO_TEST_CASE(Basic)
{
  RttEstimator rtt;

  for (int i = 0; i < 100; ++i) {
    rtt.addMeasurement(time::seconds(5));
  }
  double rto1 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_CLOSE(rto1, 5.0, 0.1);

  rtt.doubleMultiplier();
  double rto2 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_CLOSE(rto2, 10.0, 0.1);

  rtt.doubleMultiplier();
  double rto3 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_CLOSE(rto3, 20.0, 0.1);

  rtt.addMeasurement(time::seconds(5)); // reset multiplier
  double rto4 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_CLOSE(rto4, 5.0, 0.1);

  rtt.incrementMultiplier();
  double rto5 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_CLOSE(rto5, 10.0, 0.1);

  for (int i = 0; i < 5; ++i) {
    rtt.addMeasurement(time::seconds(6));
  } // increased variance
  double rto6 = computeRtoAsFloatSeconds(rtt);
  BOOST_CHECK_GT(rto6, rto1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace nfd
