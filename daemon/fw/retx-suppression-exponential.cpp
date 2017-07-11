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

RetxSuppressionResult
RetxSuppressionExponential::decidePerPitEntry(pit::Entry& pitEntry)
{
  bool isNewPitEntry = !hasPendingOutRecords(pitEntry);
  if (isNewPitEntry) {
    return RetxSuppressionResult::NEW;
  }

  time::steady_clock::TimePoint lastOutgoing = getLastOutgoing(pitEntry);
  time::steady_clock::TimePoint now = time::steady_clock::now();
  time::steady_clock::Duration sinceLastOutgoing = now - lastOutgoing;

  PitInfo* pi = pitEntry.insertStrategyInfo<PitInfo>(m_initialInterval).first;
  bool shouldSuppress = sinceLastOutgoing < pi->suppressionInterval;

  if (shouldSuppress) {
    return RetxSuppressionResult::SUPPRESS;
  }

  pi->suppressionInterval = std::min(m_maxInterval,
    time::duration_cast<Duration>(pi->suppressionInterval * m_multiplier));

  return RetxSuppressionResult::FORWARD;
}

RetxSuppressionResult
RetxSuppressionExponential::decidePerUpstream(pit::Entry& pitEntry, Face& outFace)
{
  // NEW if outRecord for the face does not exist
  auto outRecord = pitEntry.getOutRecord(outFace);
  if (outRecord == pitEntry.out_end()) {
    return RetxSuppressionResult::NEW;
  }

  time::steady_clock::TimePoint lastOutgoing = outRecord->getLastRenewed();
  time::steady_clock::TimePoint now = time::steady_clock::now();
  time::steady_clock::Duration sinceLastOutgoing = now - lastOutgoing;

  // insertStrategyInfo does not insert m_initialInterval again if it already exists
  PitInfo* pi = outRecord->insertStrategyInfo<PitInfo>(m_initialInterval).first;
  bool shouldSuppress = sinceLastOutgoing < pi->suppressionInterval;

  if (shouldSuppress) {
    return RetxSuppressionResult::SUPPRESS;
  }

  return RetxSuppressionResult::FORWARD;
}

void
RetxSuppressionExponential::incrementIntervalForOutRecord(pit::OutRecord& outRecord)
{
  PitInfo* pi = outRecord.insertStrategyInfo<PitInfo>(m_initialInterval).first;
  pi->suppressionInterval = std::min(m_maxInterval,
    time::duration_cast<Duration>(pi->suppressionInterval * m_multiplier));
}

} // namespace fw
} // namespace nfd
