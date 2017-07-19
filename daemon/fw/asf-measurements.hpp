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

#ifndef NFD_DAEMON_FW_ASF_MEASUREMENTS_HPP
#define NFD_DAEMON_FW_ASF_MEASUREMENTS_HPP

#include "core/rtt-estimator.hpp"
#include "fw/strategy-info.hpp"
#include "table/measurements-accessor.hpp"

namespace nfd {
namespace fw {
namespace asf {

class RttStats
{
public:
  typedef time::duration<double, boost::micro> Rtt;

  RttStats();

  void
  addRttMeasurement(RttEstimator::Duration& durationRtt);

  void
  recordTimeout()
  {
    m_rtt = RTT_TIMEOUT;
  }

  Rtt
  getRtt() const
  {
    return m_rtt;
  }

  Rtt
  getSrtt() const
  {
    return m_srtt;
  }

  RttEstimator::Duration
  computeRto() const
  {
    return m_rttEstimator.computeRto();
  }

private:
  static Rtt
  computeSrtt(Rtt previousSrtt, Rtt currentRtt);

public:
  static const Rtt RTT_TIMEOUT;
  static const Rtt RTT_NO_MEASUREMENT;

private:
  Rtt m_srtt;
  Rtt m_rtt;
  RttEstimator m_rttEstimator;

  static const double ALPHA;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief Strategy information for each face in a namespace
*/
class FaceInfo
{
public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  FaceInfo();

  ~FaceInfo();

  void
  setTimeoutEvent(const scheduler::EventId& id, const Name& interestName);

  void
  setMeasurementExpirationEventId(const scheduler::EventId& id)
  {
    m_measurementExpirationId = id;
  }

  const scheduler::EventId&
  getMeasurementExpirationEventId()
  {
    return m_measurementExpirationId;
  }

  void
  cancelTimeoutEvent(const Name& prefix);

  bool
  isTimeoutScheduled() const
  {
    return m_isTimeoutScheduled;
  }

  void
  recordRtt(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace);

  void
  recordTimeout(const Name& interestName);

  bool
  isTimeout() const
  {
    return getRtt() == RttStats::RTT_TIMEOUT;
  }

  RttEstimator::Duration
  computeRto() const
  {
    return m_rttStats.computeRto();
  }

  RttStats::Rtt
  getRtt() const
  {
    return m_rttStats.getRtt();
  }

  RttStats::Rtt
  getSrtt() const
  {
    return m_rttStats.getSrtt();
  }

  bool
  hasSrttMeasurement() const
  {
    return getSrtt() != RttStats::RTT_NO_MEASUREMENT;
  }

  size_t
  getNSilentTimeouts() const
  {
    return m_nSilentTimeouts;
  }

  void
  setNSilentTimeouts(size_t nSilentTimeouts)
  {
    m_nSilentTimeouts = nSilentTimeouts;
  }

private:
  void
  cancelTimeoutEvent();

  bool
  doesNameMatchLastInterest(const Name& name);

private:
  RttStats m_rttStats;
  Name m_lastInterestName;

  // Timeout associated with measurement
  scheduler::EventId m_measurementExpirationId;

  // RTO associated with Interest
  scheduler::EventId m_timeoutEventId;
  bool m_isTimeoutScheduled;
  size_t m_nSilentTimeouts;
};

typedef std::unordered_map<FaceId, FaceInfo> FaceInfoTable;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief stores stategy information about each face in this namespace
 */
class NamespaceInfo : public StrategyInfo
{
public:
  NamespaceInfo();

  static constexpr int
  getTypeId()
  {
    return 1030;
  }

  FaceInfo&
  getOrCreateFaceInfo(const fib::Entry& fibEntry, FaceId faceId);

  FaceInfo*
  getFaceInfo(const fib::Entry& fibEntry, FaceId faceId);

  void
  expireFaceInfo(FaceId faceId);

  void
  extendFaceInfoLifetime(FaceInfo& info, FaceId faceId);

  FaceInfo*
  get(FaceId faceId)
  {
    if (m_fit.find(faceId) != m_fit.end()) {
      return &m_fit.at(faceId);
    }
    else {
      return nullptr;
    }
  }

  FaceInfoTable::iterator
  find(FaceId faceId)
  {
    return m_fit.find(faceId);
  }

  FaceInfoTable::iterator
  end()
  {
    return m_fit.end();
  }

  const FaceInfoTable::iterator
  insert(FaceId faceId)
  {
    const auto& pair = m_fit.insert(std::make_pair(faceId, FaceInfo()));
    return pair.first;
  }

  bool
  isProbingDue() const
  {
    return m_isProbingDue;
  }

  void
  setIsProbingDue(bool isProbingDue)
  {
    m_isProbingDue = isProbingDue;
  }

  bool
  isFirstProbeScheduled() const
  {
    return m_hasFirstProbeBeenScheduled;
  }

  void
  setHasFirstProbeBeenScheduled(bool hasBeenScheduled)
  {
    m_hasFirstProbeBeenScheduled = hasBeenScheduled;
  }

private:
  FaceInfoTable m_fit;

  bool m_isProbingDue;
  bool m_hasFirstProbeBeenScheduled;
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/** \brief Helper class to retrieve and create strategy measurements
 */
class AsfMeasurements : noncopyable
{
public:
  explicit
  AsfMeasurements(MeasurementsAccessor& measurements);

  FaceInfo*
  getFaceInfo(const fib::Entry& fibEntry, const Interest& interest, FaceId faceId);

  FaceInfo&
  getOrCreateFaceInfo(const fib::Entry& fibEntry, const Interest& interest, FaceId faceId);

  NamespaceInfo*
  getNamespaceInfo(const Name& prefix);

  NamespaceInfo&
  getOrCreateNamespaceInfo(const fib::Entry& fibEntry, const Interest& interest);

private:
  void
  extendLifetime(measurements::Entry& me);

public:
  static constexpr time::microseconds MEASUREMENTS_LIFETIME = time::seconds(300);

private:
  MeasurementsAccessor& m_measurements;
};

} // namespace asf
} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_ASF_MEASUREMENTS_HPP
