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

#include "asf-measurements.hpp"

namespace nfd {
namespace fw {
namespace asf {

NFD_LOG_INIT("AsfMeasurements");

const RttStats::Rtt RttStats::RTT_TIMEOUT(-1.0);
const RttStats::Rtt RttStats::RTT_NO_MEASUREMENT(0.0);
const double RttStats::ALPHA = 0.125;

RttStats::RttStats()
  : m_srtt(RTT_NO_MEASUREMENT)
  , m_rtt(RTT_NO_MEASUREMENT)
{
}

void
RttStats::addRttMeasurement(RttEstimator::Duration& durationRtt)
{
  m_rtt = static_cast<RttStats::Rtt>(durationRtt.count());

  m_rttEstimator.addMeasurement(durationRtt);

  m_srtt = computeSrtt(m_srtt, m_rtt);
}

RttStats::Rtt
RttStats::computeSrtt(Rtt previousSrtt, Rtt currentRtt)
{
  if (previousSrtt == RTT_NO_MEASUREMENT) {
    return currentRtt;
  }

  return Rtt(ALPHA * currentRtt + (1 - ALPHA) * previousSrtt);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

FaceInfo::FaceInfo()
  : m_isTimeoutScheduled(false)
  , m_nSilentTimeouts(0)
{
}

FaceInfo::~FaceInfo()
{
  cancelTimeoutEvent();
  scheduler::cancel(m_measurementExpirationId);
}

void
FaceInfo::setTimeoutEvent(const scheduler::EventId& id, const Name& interestName)
{
  if (!m_isTimeoutScheduled) {
    m_timeoutEventId = id;
    m_isTimeoutScheduled = true;
    m_lastInterestName = interestName;
  }
  else {
    BOOST_THROW_EXCEPTION(FaceInfo::Error("Tried to schedule a timeout for a face that already has a timeout scheduled."));
  }
}

void
FaceInfo::cancelTimeoutEvent()
{
  scheduler::cancel(m_timeoutEventId);
  m_isTimeoutScheduled = false;
}

void
FaceInfo::cancelTimeoutEvent(const Name& prefix)
{
  if (isTimeoutScheduled() && doesNameMatchLastInterest(prefix)) {
    cancelTimeoutEvent();
  }
}

bool
FaceInfo::doesNameMatchLastInterest(const Name& name)
{
  return m_lastInterestName.isPrefixOf(name);
}

void
FaceInfo::recordRtt(const shared_ptr<pit::Entry>& pitEntry, const Face& inFace)
{
  // Calculate RTT
  pit::OutRecordCollection::const_iterator outRecord = pitEntry->getOutRecord(inFace);

  if (outRecord == pitEntry->out_end()) { // no out-record
    NFD_LOG_TRACE(pitEntry->getInterest() << " dataFrom inFace=" << inFace.getId() << " no-out-record");
    return;
  }

  time::steady_clock::Duration steadyRtt = time::steady_clock::now() - outRecord->getLastRenewed();
  RttEstimator::Duration durationRtt = time::duration_cast<RttEstimator::Duration>(steadyRtt);

  m_rttStats.addRttMeasurement(durationRtt);

  NFD_LOG_TRACE("Recording RTT for FaceId: " << inFace.getId()
                                             << " RTT: "    << m_rttStats.getRtt()
                                             << " SRTT: "   << m_rttStats.getSrtt());
}

void
FaceInfo::recordTimeout(const Name& interestName)
{
  m_rttStats.recordTimeout();
  cancelTimeoutEvent(interestName);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

NamespaceInfo::NamespaceInfo()
  : m_isProbingDue(false)
  , m_hasFirstProbeBeenScheduled(false)
{
}

FaceInfo*
NamespaceInfo::getFaceInfo(const fib::Entry& fibEntry, FaceId faceId)
{
  FaceInfoTable::iterator it = m_fit.find(faceId);

  if (it != m_fit.end()) {
    return &it->second;
  }
  else {
    return nullptr;
  }
}

FaceInfo&
NamespaceInfo::getOrCreateFaceInfo(const fib::Entry& fibEntry, FaceId faceId)
{
  FaceInfoTable::iterator it = m_fit.find(faceId);

  FaceInfo* info = nullptr;

  if (it == m_fit.end()) {
    const auto& pair = m_fit.insert(std::make_pair(faceId, FaceInfo()));
    info = &pair.first->second;

    extendFaceInfoLifetime(*info, faceId);
  }
  else {
    info = &it->second;
  }

  return *info;
}

void
NamespaceInfo::expireFaceInfo(FaceId faceId)
{
  m_fit.erase(faceId);
}

void
NamespaceInfo::extendFaceInfoLifetime(FaceInfo& info, FaceId faceId)
{
  // Cancel previous expiration
  scheduler::cancel(info.getMeasurementExpirationEventId());

  // Refresh measurement
  scheduler::EventId id = scheduler::schedule(AsfMeasurements::MEASUREMENTS_LIFETIME,
    bind(&NamespaceInfo::expireFaceInfo, this, faceId));

  info.setMeasurementExpirationEventId(id);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

constexpr time::microseconds AsfMeasurements::MEASUREMENTS_LIFETIME;

AsfMeasurements::AsfMeasurements(MeasurementsAccessor& measurements)
  : m_measurements(measurements)
{
}

FaceInfo*
AsfMeasurements::getFaceInfo(const fib::Entry& fibEntry, const Interest& interest, FaceId faceId)
{
  NamespaceInfo& info = getOrCreateNamespaceInfo(fibEntry, interest);
  return info.getFaceInfo(fibEntry, faceId);
}

FaceInfo&
AsfMeasurements::getOrCreateFaceInfo(const fib::Entry& fibEntry, const Interest& interest,
                                     FaceId faceId)
{
  NamespaceInfo& info = getOrCreateNamespaceInfo(fibEntry, interest);
  return info.getOrCreateFaceInfo(fibEntry, faceId);
}

NamespaceInfo*
AsfMeasurements::getNamespaceInfo(const Name& prefix)
{
  measurements::Entry* me = m_measurements.findLongestPrefixMatch(prefix);
  if (me == nullptr) {
    return nullptr;
  }

  // Set or update entry lifetime
  extendLifetime(*me);

  NamespaceInfo* info = me->insertStrategyInfo<NamespaceInfo>().first;
  BOOST_ASSERT(info != nullptr);
  return info;
}

NamespaceInfo&
AsfMeasurements::getOrCreateNamespaceInfo(const fib::Entry& fibEntry, const Interest& interest)
{
  measurements::Entry* me = m_measurements.get(fibEntry);

  // If the FIB entry is not under the strategy's namespace, find a part of the prefix
  // that falls under the strategy's namespace
  for (size_t prefixLen = fibEntry.getPrefix().size() + 1;
       me == nullptr && prefixLen <= interest.getName().size(); ++prefixLen) {
    me = m_measurements.get(interest.getName().getPrefix(prefixLen));
  }

  // Either the FIB entry or the Interest's name must be under this strategy's namespace
  BOOST_ASSERT(me != nullptr);

  // Set or update entry lifetime
  extendLifetime(*me);

  NamespaceInfo* info = me->insertStrategyInfo<NamespaceInfo>().first;
  BOOST_ASSERT(info != nullptr);
  return *info;
}

void
AsfMeasurements::extendLifetime(measurements::Entry& me)
{
  m_measurements.extendLifetime(me, MEASUREMENTS_LIFETIME);
}

} // namespace asf
} // namespace fw
} // namespace nfd
