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

#include "retx-suppression-exponential.hpp"

namespace nfd {
namespace fw {

const RetxSuppressionExponential::Duration RetxSuppressionExponential::DEFAULT_INITIAL_INTERVAL =
    time::milliseconds(1);
const float RetxSuppressionExponential::DEFAULT_MULTIPLIER = 2.0;
const RetxSuppressionExponential::Duration RetxSuppressionExponential::DEFAULT_MAX_INTERVAL =
    time::milliseconds(250);

class RetxSuppressionExponential::PitInfo : public StrategyInfo
{
public:
  static constexpr int
  getTypeId()
  {
    return 1020;
  }

  explicit
  PitInfo(const Duration& initialInterval)
    : suppressionInterval(initialInterval)
  {
  }

public:
  /** \brief if last transmission occurred within suppressionInterval,
   *         retransmission will be suppressed
   */
  Duration suppressionInterval;
};

RetxSuppressionExponential::RetxSuppressionExponential(const Duration& initialInterval,
                                                       float multiplier,
                                                       const Duration& maxInterval)
  : m_initialInterval(initialInterval)
  , m_multiplier(multiplier)
  , m_maxInterval(maxInterval)
{
  BOOST_ASSERT(initialInterval > time::milliseconds::zero());
  BOOST_ASSERT(multiplier >= 1.0);
  BOOST_ASSERT(maxInterval >= initialInterval);
}

RetxSuppression::Result
RetxSuppressionExponential::decide(const Face& inFace, const Interest& interest,
                                   pit::Entry& pitEntry) const
{
  bool isNewPitEntry = !pitEntry.hasUnexpiredOutRecords();
  if (isNewPitEntry) {
    return NEW;
  }

  time::steady_clock::TimePoint lastOutgoing = this->getLastOutgoing(pitEntry);
  time::steady_clock::TimePoint now = time::steady_clock::now();
  time::steady_clock::Duration sinceLastOutgoing = now - lastOutgoing;

  shared_ptr<PitInfo> pi = pitEntry.getOrCreateStrategyInfo<PitInfo>(m_initialInterval);
  bool shouldSuppress = sinceLastOutgoing < pi->suppressionInterval;

  if (shouldSuppress) {
    return SUPPRESS;
  }

  pi->suppressionInterval = std::min(m_maxInterval,
      time::duration_cast<Duration>(pi->suppressionInterval * m_multiplier));
  return FORWARD;
}

} // namespace fw
} // namespace nfd
