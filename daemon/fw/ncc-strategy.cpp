/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "ncc-strategy.hpp"

namespace nfd {
namespace fw {

const Name NccStrategy::STRATEGY_NAME("ndn:/localhost/nfd/strategy/ncc");

NccStrategy::NccStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder, name)
{
}

NccStrategy::~NccStrategy()
{
}

const time::Duration NccStrategy::DEFER_FIRST_WITHOUT_BEST_FACE = time::microseconds(4000);
const time::Duration NccStrategy::DEFER_RANGE_WITHOUT_BEST_FACE = time::microseconds(75000);
const time::Duration NccStrategy::MEASUREMENTS_LIFETIME = time::seconds(16);

void
NccStrategy::afterReceiveInterest(const Face& inFace,
                                  const Interest& interest,
                                  shared_ptr<fib::Entry> fibEntry,
                                  shared_ptr<pit::Entry> pitEntry)
{
  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  if (nexthops.size() == 0) {
    this->rejectPendingInterest(pitEntry);
    return;
  }

  shared_ptr<PitEntryInfo> pitEntryInfo =
    pitEntry->getOrCreateStrategyInfo<PitEntryInfo>();
  bool isNewInterest = pitEntryInfo->m_isNewInterest;
  if (!isNewInterest) {
    return;
  }
  pitEntryInfo->m_isNewInterest = false;

  shared_ptr<MeasurementsEntryInfo> measurementsEntryInfo =
    this->getMeasurementsEntryInfo(pitEntry);

  time::Duration deferFirst = DEFER_FIRST_WITHOUT_BEST_FACE;
  time::Duration deferRange = DEFER_RANGE_WITHOUT_BEST_FACE;
  size_t nUpstreams = nexthops.size();

  shared_ptr<Face> bestFace = measurementsEntryInfo->getBestFace();
  if (static_cast<bool>(bestFace) && fibEntry->hasNextHop(bestFace) &&
      pitEntry->canForwardTo(bestFace)) {
    // TODO Should we use `randlow = 100 + nrand48(h->seed) % 4096U;` ?
    deferFirst = measurementsEntryInfo->m_prediction;
    deferRange = static_cast<time::Duration>((deferFirst + time::microseconds(1)) / 2);
    --nUpstreams;
    this->sendInterest(pitEntry, bestFace);
    pitEntryInfo->m_bestFaceTimeout = scheduler::schedule(
      measurementsEntryInfo->m_prediction,
      bind(&NccStrategy::timeoutOnBestFace, this, weak_ptr<pit::Entry>(pitEntry)));
  }
  else {
    // use first nexthop
    this->sendInterest(pitEntry, nexthops.begin()->getFace());
    // TODO avoid sending to inFace
  }

  shared_ptr<Face> previousFace = measurementsEntryInfo->m_previousFace.lock();
  if (static_cast<bool>(previousFace) && fibEntry->hasNextHop(previousFace) &&
      pitEntry->canForwardTo(previousFace)) {
    --nUpstreams;
  }

  pitEntryInfo->m_maxInterval = std::max(time::microseconds(1),
    static_cast<time::Duration>((2 * deferRange + nUpstreams - 1) / nUpstreams));
  pitEntryInfo->m_propagateTimer = scheduler::schedule(deferFirst,
    bind(&NccStrategy::doPropagate, this,
         weak_ptr<pit::Entry>(pitEntry), weak_ptr<fib::Entry>(fibEntry)));
}

void
NccStrategy::doPropagate(weak_ptr<pit::Entry> pitEntryWeak, weak_ptr<fib::Entry> fibEntryWeak)
{
  shared_ptr<pit::Entry> pitEntry = pitEntryWeak.lock();
  if (!static_cast<bool>(pitEntry)) {
    return;
  }
  shared_ptr<fib::Entry> fibEntry = fibEntryWeak.lock();
  if (!static_cast<bool>(fibEntry)) {
    return;
  }

  shared_ptr<PitEntryInfo> pitEntryInfo =
    pitEntry->getStrategyInfo<PitEntryInfo>();
  BOOST_ASSERT(static_cast<bool>(pitEntryInfo));

  shared_ptr<MeasurementsEntryInfo> measurementsEntryInfo =
    this->getMeasurementsEntryInfo(pitEntry);

  shared_ptr<Face> previousFace = measurementsEntryInfo->m_previousFace.lock();
  if (static_cast<bool>(previousFace) && fibEntry->hasNextHop(previousFace) &&
      pitEntry->canForwardTo(previousFace)) {
    this->sendInterest(pitEntry, previousFace);
  }

  const fib::NextHopList& nexthops = fibEntry->getNextHops();
  bool isForwarded = false;
  for (fib::NextHopList::const_iterator it = nexthops.begin(); it != nexthops.end(); ++it) {
    shared_ptr<Face> face = it->getFace();
    if (pitEntry->canForwardTo(face)) {
      isForwarded = true;
      this->sendInterest(pitEntry, face);
      break;
    }
  }

  if (isForwarded) {
    static unsigned short seed[3];
    time::Duration deferNext = static_cast<time::Duration>(
                               nrand48(seed) % pitEntryInfo->m_maxInterval);
    pitEntryInfo->m_propagateTimer = scheduler::schedule(deferNext,
    bind(&NccStrategy::doPropagate, this,
         weak_ptr<pit::Entry>(pitEntry), weak_ptr<fib::Entry>(fibEntry)));
  }
}

void
NccStrategy::timeoutOnBestFace(weak_ptr<pit::Entry> pitEntryWeak)
{
  shared_ptr<pit::Entry> pitEntry = pitEntryWeak.lock();
  if (!static_cast<bool>(pitEntry)) {
    return;
  }
  shared_ptr<measurements::Entry> measurementsEntry = this->getMeasurements().get(*pitEntry);

  for (int i = 0; i < UPDATE_MEASUREMENTS_N_LEVELS; ++i) {
    if (!static_cast<bool>(measurementsEntry)) {
      // going out of this strategy's namespace
      return;
    }
    this->getMeasurements().extendLifetime(*measurementsEntry, MEASUREMENTS_LIFETIME);

    shared_ptr<MeasurementsEntryInfo> measurementsEntryInfo =
      this->getMeasurementsEntryInfo(measurementsEntry);
    measurementsEntryInfo->adjustPredictUp();

    measurementsEntry = this->getMeasurements().getParent(measurementsEntry);
  }
}

void
NccStrategy::beforeSatisfyPendingInterest(shared_ptr<pit::Entry> pitEntry,
                                          const Face& inFace, const Data& data)
{
  shared_ptr<measurements::Entry> measurementsEntry = this->getMeasurements().get(*pitEntry);

  for (int i = 0; i < UPDATE_MEASUREMENTS_N_LEVELS; ++i) {
    if (!static_cast<bool>(measurementsEntry)) {
      // going out of this strategy's namespace
      return;
    }
    this->getMeasurements().extendLifetime(*measurementsEntry, MEASUREMENTS_LIFETIME);

    shared_ptr<MeasurementsEntryInfo> measurementsEntryInfo =
      this->getMeasurementsEntryInfo(measurementsEntry);
    measurementsEntryInfo->updateBestFace(inFace);

    measurementsEntry = this->getMeasurements().getParent(measurementsEntry);
  }

  shared_ptr<PitEntryInfo> pitEntryInfo =
    pitEntry->getStrategyInfo<PitEntryInfo>();
  scheduler::cancel(pitEntryInfo->m_propagateTimer);
}

shared_ptr<NccStrategy::MeasurementsEntryInfo>
NccStrategy::getMeasurementsEntryInfo(shared_ptr<pit::Entry> entry)
{
  shared_ptr<measurements::Entry> measurementsEntry = this->getMeasurements().get(*entry);
  return this->getMeasurementsEntryInfo(measurementsEntry);
}

shared_ptr<NccStrategy::MeasurementsEntryInfo>
NccStrategy::getMeasurementsEntryInfo(shared_ptr<measurements::Entry> entry)
{
  shared_ptr<MeasurementsEntryInfo> info = entry->getStrategyInfo<MeasurementsEntryInfo>();
  if (static_cast<bool>(info)) {
    return info;
  }

  info = make_shared<MeasurementsEntryInfo>();
  entry->setStrategyInfo(info);

  shared_ptr<measurements::Entry> parentEntry = this->getMeasurements().getParent(entry);
  if (static_cast<bool>(parentEntry)) {
    shared_ptr<MeasurementsEntryInfo> parentInfo = this->getMeasurementsEntryInfo(parentEntry);
    BOOST_ASSERT(static_cast<bool>(parentInfo));
    info->inheritFrom(*parentInfo);
  }

  return info;
}


const time::Duration NccStrategy::MeasurementsEntryInfo::INITIAL_PREDICTION =
                                                         time::microseconds(8192);
const time::Duration NccStrategy::MeasurementsEntryInfo::MIN_PREDICTION =
                                                         time::microseconds(127);
const time::Duration NccStrategy::MeasurementsEntryInfo::MAX_PREDICTION =
                                                         time::microseconds(160000);

NccStrategy::MeasurementsEntryInfo::MeasurementsEntryInfo()
  : m_prediction(INITIAL_PREDICTION)
{
}

void
NccStrategy::MeasurementsEntryInfo::inheritFrom(const MeasurementsEntryInfo& other)
{
  this->operator=(other);
}

shared_ptr<Face>
NccStrategy::MeasurementsEntryInfo::getBestFace(void) {
  shared_ptr<Face> best = m_bestFace.lock();
  if (static_cast<bool>(best)) {
    return best;
  }
  m_bestFace = best = m_previousFace.lock();
  return best;
}

void
NccStrategy::MeasurementsEntryInfo::updateBestFace(const Face& face) {
  if (m_bestFace.expired()) {
    m_bestFace = const_cast<Face&>(face).shared_from_this();
    return;
  }
  shared_ptr<Face> bestFace = m_bestFace.lock();
  if (bestFace.get() == &face) {
    this->adjustPredictDown();
  }
  else {
    m_previousFace = m_bestFace;
    m_bestFace = const_cast<Face&>(face).shared_from_this();
  }
}

void
NccStrategy::MeasurementsEntryInfo::adjustPredictDown() {
  m_prediction = std::max(MIN_PREDICTION,
    static_cast<time::Duration>(m_prediction - (m_prediction >> ADJUST_PREDICT_DOWN_SHIFT)));
}

void
NccStrategy::MeasurementsEntryInfo::adjustPredictUp() {
  m_prediction = std::min(MAX_PREDICTION,
    static_cast<time::Duration>(m_prediction + (m_prediction >> ADJUST_PREDICT_UP_SHIFT)));
}

void
NccStrategy::MeasurementsEntryInfo::ageBestFace() {
  m_previousFace = m_bestFace;
  m_bestFace.reset();
}

NccStrategy::PitEntryInfo::PitEntryInfo()
  : m_isNewInterest(true)
{
}

NccStrategy::PitEntryInfo::~PitEntryInfo()
{
  scheduler::cancel(m_bestFaceTimeout);
  scheduler::cancel(m_propagateTimer);
}

} // namespace fw
} // namespace nfd
