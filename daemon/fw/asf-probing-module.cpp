/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

namespace nfd {
namespace fw {
namespace asf {

constexpr time::milliseconds ProbingModule::DEFAULT_PROBING_INTERVAL;
constexpr time::milliseconds ProbingModule::MIN_PROBING_INTERVAL;

static_assert(ProbingModule::DEFAULT_PROBING_INTERVAL < AsfMeasurements::MEASUREMENTS_LIFETIME,
              "ProbingModule::DEFAULT_PROBING_INTERVAL must be less than AsfMeasurements::MEASUREMENTS_LIFETIME");

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

Face*
ProbingModule::getFaceToProbe(const Face& inFace, const Interest& interest,
                              const fib::Entry& fibEntry, const Face& faceUsed)
{
  FaceInfoFacePairSet rankedFaces;

  // Put eligible faces into rankedFaces. If a face does not have an RTT measurement,
  // immediately pick the face for probing
  for (const auto& hop : fibEntry.getNextHops()) {
    Face& hopFace = hop.getFace();

    // Don't send probe Interest back to the incoming face or use the same face
    // as the forwarded Interest or use a face that violates scope
    if (hopFace.getId() == inFace.getId() || hopFace.getId() == faceUsed.getId() ||
        wouldViolateScope(inFace, interest, hopFace)) {
      continue;
    }

    FaceInfo* info = m_measurements.getFaceInfo(fibEntry, interest, hopFace.getId());
    // If no RTT has been recorded, probe this face
    if (info == nullptr || info->getLastRtt() == FaceInfo::RTT_NO_MEASUREMENT) {
      return &hopFace;
    }

    // Add FaceInfo to container sorted by RTT
    rankedFaces.insert({info, &hopFace});
  }

  if (rankedFaces.empty()) {
    // No Face to probe
    return nullptr;
  }

  return chooseFace(rankedFaces);
}

bool
ProbingModule::isProbingNeeded(const fib::Entry& fibEntry, const Interest& interest)
{
  // Return the probing status flag for a namespace
  NamespaceInfo& info = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);

  // If a first probe has not been scheduled for a namespace
  if (!info.isFirstProbeScheduled()) {
    // Schedule first probe between 0 and 5 seconds
    static std::uniform_int_distribution<> randDist(0, 5000);
    auto interval = randDist(ndn::random::getRandomNumberEngine());
    scheduleProbe(fibEntry, time::milliseconds(interval));
    info.setIsFirstProbeScheduled(true);
  }

  return info.isProbingDue();
}

void
ProbingModule::afterForwardingProbe(const fib::Entry& fibEntry, const Interest& interest)
{
  // After probing is done, need to set probing flag to false and
  // schedule another future probe
  NamespaceInfo& info = m_measurements.getOrCreateNamespaceInfo(fibEntry, interest);
  info.setIsProbingDue(false);

  scheduleProbe(fibEntry, m_probingInterval);
}

Face*
ProbingModule::chooseFace(const FaceInfoFacePairSet& rankedFaces)
{
  static std::uniform_real_distribution<> randDist;
  double randomNumber = randDist(ndn::random::getRandomNumberEngine());
  uint64_t rankSum = (rankedFaces.size() + 1) * rankedFaces.size() / 2;

  uint64_t rank = 1;
  double offset = 0.0;

  for (const auto& pair : rankedFaces) {
    double probability = getProbingProbability(rank++, rankSum, rankedFaces.size());

    // Is the random number within the bounds of this face's probability + the previous faces'
    // probability?
    //
    // e.g. (FaceId: 1, p=0.5), (FaceId: 2, p=0.33), (FaceId: 3, p=0.17)
    //      randomNumber = 0.92
    //
    //      The face with FaceId: 3 should be picked
    //      (0.68 < 0.5 + 0.33 + 0.17) == true
    //
    if (randomNumber <= offset + probability) {
      // Found face to probe
      return pair.second;
    }
    offset += probability;
  }

  // Given a set of Faces, this method should always select a Face to probe
  NDN_CXX_UNREACHABLE;
}

double
ProbingModule::getProbingProbability(uint64_t rank, uint64_t rankSum, uint64_t nFaces)
{
  // p = n + 1 - j ; n: # faces
  //     ---------
  //     sum(ranks)
  return static_cast<double>(nFaces + 1 - rank) / rankSum;
}

void
ProbingModule::setProbingInterval(size_t probingInterval)
{
  if (time::milliseconds(probingInterval) >= MIN_PROBING_INTERVAL) {
    m_probingInterval = time::milliseconds(probingInterval);
  }
  else {
    NDN_THROW(std::invalid_argument("Probing interval must be >= " +
                                    to_string(MIN_PROBING_INTERVAL.count()) + " milliseconds"));
  }
}

} // namespace asf
} // namespace fw
} // namespace nfd
