/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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
#include "algorithm.hpp"

namespace nfd::fw {

namespace {

class PitInfo final : public StrategyInfo
{
public:
  static constexpr int
  getTypeId()
  {
    return 1020;
  }

  explicit
  PitInfo(const RetxSuppressionExponential::Duration& initialInterval)
    : suppressionInterval(initialInterval)
  {
  }

public:
  // If the last transmission occurred within this interval, retx will be suppressed
  RetxSuppressionExponential::Duration suppressionInterval;
};

} // namespace

RetxSuppressionExponential::RetxSuppressionExponential(Duration initialInterval,
                                                       Duration maxInterval,
                                                       float multiplier)
  : m_initialInterval(initialInterval)
  , m_maxInterval(maxInterval)
  , m_multiplier(multiplier)
{
  if (m_initialInterval <= 0_ns) {
    NDN_THROW(std::invalid_argument("Retx suppression initial interval must be > 0"));
  }
  if (m_maxInterval < m_initialInterval) {
    NDN_THROW(std::invalid_argument("Retx suppression max interval must be >= initial interval"));
  }
  if (m_multiplier < 1.0f) {
    NDN_THROW(std::invalid_argument("Retx suppression multiplier must be >= 1"));
  }
}

RetxSuppressionResult
RetxSuppressionExponential::decidePerPitEntry(pit::Entry& pitEntry)
{
  bool isNewPitEntry = !hasPendingOutRecords(pitEntry);
  if (isNewPitEntry) {
    return RetxSuppressionResult::NEW;
  }

  auto lastOutgoing = getLastOutgoing(pitEntry);
  auto now = time::steady_clock::now();
  auto sinceLastOutgoing = now - lastOutgoing;

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

  auto lastOutgoing = outRecord->getLastRenewed();
  auto now = time::steady_clock::now();
  auto sinceLastOutgoing = now - lastOutgoing;

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

std::unique_ptr<RetxSuppressionExponential>
RetxSuppressionExponential::construct(const StrategyParameters& params)
{
  auto init = params.getOrDefault<Duration::rep>("retx-suppression-initial",
                                                 RetxSuppressionExponential::DEFAULT_INITIAL_INTERVAL.count());
  auto max = params.getOrDefault<Duration::rep>("retx-suppression-max",
                                                RetxSuppressionExponential::DEFAULT_MAX_INTERVAL.count());
  auto mult = params.getOrDefault<float>("retx-suppression-multiplier",
                                         RetxSuppressionExponential::DEFAULT_MULTIPLIER);

  return make_unique<RetxSuppressionExponential>(Duration(init), Duration(max), mult);
}

} // namespace nfd::fw
