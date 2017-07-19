/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP
#define NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP

#include "asf-measurements.hpp"

namespace nfd {
namespace fw {
namespace asf {

/** \brief ASF Probing Module
 */
class ProbingModule
{
public:
  explicit
  ProbingModule(AsfMeasurements& measurements);

  void
  scheduleProbe(const fib::Entry& fibEntry, const time::milliseconds& interval);

  Face*
  getFaceToProbe(const Face& inFace,
                 const Interest& interest,
                 const fib::Entry& fibEntry,
                 const Face& faceUsed);

  bool
  isProbingNeeded(const fib::Entry& fibEntry, const Interest& interest);

  void
  afterForwardingProbe(const fib::Entry& fibEntry, const Interest& interest);

  void
  setProbingInterval(size_t probingInterval);

  time::milliseconds
  getProbingInterval() const
  {
    return m_probingInterval;
  }

private:
  // Used to associate FaceInfo with the face in a NextHop
  typedef std::pair<FaceInfo*, Face*> FaceInfoFacePair;
  typedef std::function<bool(FaceInfoFacePair, FaceInfoFacePair)> FaceInfoPredicate;
  typedef std::set<FaceInfoFacePair, FaceInfoPredicate> FaceInfoFacePairSet;

  Face*
  getFaceBasedOnProbability(const FaceInfoFacePairSet& rankedFaces);

  double
  getProbingProbability(uint64_t rank, uint64_t rankSum, uint64_t nFaces);

  double
  getRandomNumber(double start, double end);

public:
  static constexpr time::milliseconds DEFAULT_PROBING_INTERVAL = time::milliseconds(60000);
  static constexpr time::milliseconds MIN_PROBING_INTERVAL = time::milliseconds(1000);

private:
  time::milliseconds m_probingInterval;
  AsfMeasurements& m_measurements;
};

} // namespace asf
} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ASF_PROBING_MODULE_HPP
