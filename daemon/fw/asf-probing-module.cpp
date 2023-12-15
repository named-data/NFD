/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2024,  Regents of the University of California,
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

#include "asf-probing-module.hpp"
#include "algorithm.hpp"
#include "common/global.hpp"

#include <ndn-cxx/util/random.hpp>

namespace nfd::fw::asf {

static_assert(ProbingModule::DEFAULT_PROBING_INTERVAL < AsfMeasurements::MEASUREMENTS_LIFETIME);

ProbingModule::ProbingModule(AsfMeasurements& measurements)
  : m_probingInterval(DEFAULT_PROBING_INTERVAL)
  , m_measurements(measurements)
{
}

void
ProbingModule::scheduleProbe(const fib::Entry& fibEntry, time::milliseconds interval)
{
  Name prefix = fibEntry.getPrefix();

  // Set the probing flag for the namespace to true after passed interval of time
  getScheduler().schedule(interval, [this, prefix] {
    NamespaceInfo* info = m_measurements.getNamespaceInfo(prefix);
    if (info == nullptr) {
      // FIB entry with the passed prefix has been removed or
      // it has a name that is not controlled by the AsfStrategy
    }
    else {
      info->setIsProbingDue(true);
    }
  });
}

static auto
getFaceRankForProbing(const FaceStats& fs) noexcept
{
  // The RTT is used to store the status of the face:
  //  - A positive value indicates data was received and is assumed to indicate a working face (group 1),
  //  - RTT_NO_MEASUREMENT indicates a face is unmeasured (group 2),
  //  - RTT_TIMEOUT indicates a face is timed out (group 3).
  // These groups are defined in the technical report.
  //
  // Unlike during forwarding, we adjust the ranking such that unmeasured faces (group 2)
  // are prioritized before working faces (group 1), and working faces are prioritized
  // before timed out faces (group 3). We assign each group a priority value from 1-3
  // to ensure lowest-to-highest ordering consistent with this logic.
  // Additionally, unmeasured faces will always be chosen to probe if they exist.

  // Working faces are ranked second in priority; if RTT is not
  // a special value, we assume the face to be in this group.
  int priority = 2;
  if (fs.rtt == FaceInfo::RTT_NO_MEASUREMENT) {
    priority = 1;
  }
  else if (fs.rtt == FaceInfo::RTT_TIMEOUT) {
    priority = 3;
  }

  // We set SRTT by default to the max value; if a face is working, we instead set it to the actual value.
  // Unmeasured and timed out faces are not sorted by SRTT.
  auto srtt = priority == 2 ? fs.srtt : time::nanoseconds::max();

  // For ranking, group takes the priority over SRTT (if present) or cost, SRTT (if present)
  // takes priority over cost, and cost takes priority over FaceId.
  // FaceId is included to ensure all unique entries are included in the ranking (see #5310).
  return std::tuple(priority, srtt, fs.cost, fs.face->getId());
}

bool
ProbingModule::FaceStatsProbingCompare::operator()(const FaceStats& lhs, const FaceStats& rhs) const noexcept
{
  return getFaceRankForProbing(lhs) < getFaceRankForProbing(rhs);
}

static Face*
chooseFace(const ProbingModule::FaceStatsProbingSet& rankedFaces)
{
  static std::uniform_real_distribution<> randDist;
  static auto& rng = ndn::random::getRandomNumberEngine();
  const double randomNumber = randDist(rng);

  const auto nFaces = rankedFaces.size();
  const double rankSum = (nFaces + 1) * nFaces / 2;
  size_t rank = 1;
  double offset = 0.0;

  for (const auto& faceStat : rankedFaces) {
    //     n + 1 - j
    // p = ---------
    //     sum(ranks)
    double probability = static_cast<double>(nFaces + 1 - rank) / rankSum;
    rank++;

    // Is the random number within the bounds of this face's probability + the previous faces'
    // probability?
    //
    // e.g. (FaceId: 1, p=0.5), (FaceId: 2, p=0.33), (FaceId: 3, p=0.17)
    //      randomNumber = 0.92
    //
    //      The face with FaceId: 3 should be picked
    //      (0.68 < 0.5 + 0.33 + 0.17) == true
    //
    offset += probability;
    if (randomNumber <= offset) {
      // Found face to probe
      return faceStat.face;
    }
  }

  // Given a set of Faces, this method should always select a Face to probe
  NDN_CXX_UNREACHABLE;
}

Face*
ProbingModule::getFaceToProbe(const Face& inFace, const Interest& interest,
                              const fib::Entry& fibEntry, const Face& faceUsed)
{
  FaceStatsProbingSet rankedFaces;

  // Put eligible faces into rankedFaces. If one or more faces do not have an RTT measurement,
  // the lowest ranked one will always be returned.
  for (const auto& hop : fibEntry.getNextHops()) {
    Face& hopFace = hop.getFace();

    // Don't send probe Interest back to the incoming face or use the same face
    // as the forwarded Interest or use a face that violates scope
    if (hopFace.getId() == inFace.getId() || hopFace.getId() == faceUsed.getId() ||
        wouldViolateScope(inFace, interest, hopFace)) {
      continue;
    }

    FaceInfo* info = m_measurements.getFaceInfo(fibEntry, interest.getName(), hopFace.getId());
    if (info == nullptr || info->getLastRtt() == FaceInfo::RTT_NO_MEASUREMENT) {
      rankedFaces.insert({&hopFace, FaceInfo::RTT_NO_MEASUREMENT,
                          FaceInfo::RTT_NO_MEASUREMENT, hop.getCost()});
    }
    else {
      rankedFaces.insert({&hopFace, info->getLastRtt(), info->getSrtt(), hop.getCost()});
    }
  }

  if (rankedFaces.empty()) {
    // No Face to probe
    return nullptr;
  }

  // If the top face is unmeasured, immediately return it.
  if (rankedFaces.begin()->rtt == FaceInfo::RTT_NO_MEASUREMENT) {
    return rankedFaces.begin()->face;
  }

  return chooseFace(rankedFaces);
}

bool
ProbingModule::isProbingNeeded(const fib::Entry& fibEntry, const Name& interestName)
{
  // Return the probing status flag for a namespace
  NamespaceInfo& info = m_measurements.getOrCreateNamespaceInfo(fibEntry, interestName);

  // If a first probe has not been scheduled for a namespace
  if (!info.isFirstProbeScheduled()) {
    // Schedule first probe between 0 and 5 seconds
    static std::uniform_int_distribution<> randDist(0, 5000);
    static auto& rng = ndn::random::getRandomNumberEngine();
    auto interval = randDist(rng);
    scheduleProbe(fibEntry, time::milliseconds(interval));
    info.setIsFirstProbeScheduled(true);
  }

  return info.isProbingDue();
}

void
ProbingModule::afterForwardingProbe(const fib::Entry& fibEntry, const Name& interestName)
{
  // After probing is done, need to set probing flag to false and
  // schedule another future probe
  NamespaceInfo& info = m_measurements.getOrCreateNamespaceInfo(fibEntry, interestName);
  info.setIsProbingDue(false);

  scheduleProbe(fibEntry, m_probingInterval);
}

void
ProbingModule::setProbingInterval(time::milliseconds probingInterval)
{
  if (probingInterval >= MIN_PROBING_INTERVAL) {
    m_probingInterval = probingInterval;
  }
  else {
    NDN_THROW(std::invalid_argument("Probing interval must be >= " +
                                    std::to_string(MIN_PROBING_INTERVAL.count()) + " milliseconds"));
  }
}

} // namespace nfd::fw::asf
