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

#ifndef NFD_DAEMON_FW_NCC_STRATEGY_HPP
#define NFD_DAEMON_FW_NCC_STRATEGY_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/** \brief a forwarding strategy similar to CCNx 0.7.2
 */
class NccStrategy : public Strategy
{
public:
  NccStrategy(Forwarder& forwarder, const Name& name = STRATEGY_NAME);

  virtual
  ~NccStrategy();

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry) DECL_OVERRIDE;

  virtual void
  beforeSatisfyInterest(shared_ptr<pit::Entry> pitEntry,
                        const Face& inFace, const Data& data) DECL_OVERRIDE;

protected:
  /// StrategyInfo on measurements::Entry
  class MeasurementsEntryInfo : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1000;
    }

    MeasurementsEntryInfo();

    void
    inheritFrom(const MeasurementsEntryInfo& other);

    shared_ptr<Face>
    getBestFace();

    void
    updateBestFace(const Face& face);

    void
    adjustPredictUp();

  private:
    void
    adjustPredictDown();

    void
    ageBestFace();

  public:
    weak_ptr<Face> bestFace;
    weak_ptr<Face> previousFace;
    time::microseconds prediction;

    static const time::microseconds INITIAL_PREDICTION;
    static const time::microseconds MIN_PREDICTION;
    static const int ADJUST_PREDICT_DOWN_SHIFT = 7;
    static const time::microseconds MAX_PREDICTION;
    static const int ADJUST_PREDICT_UP_SHIFT = 3;
  };

  /// StrategyInfo on pit::Entry
  class PitEntryInfo : public StrategyInfo
  {
  public:
    static constexpr int
    getTypeId()
    {
      return 1001;
    }

    virtual
    ~PitEntryInfo();

  public:
    /// timer that expires when best face does not respond within predicted time
    scheduler::EventId bestFaceTimeout;
    /// timer for propagating to another face
    scheduler::EventId propagateTimer;
    /// maximum interval between forwarding to two nexthops except best and previous
    time::microseconds maxInterval;
  };

protected:
  shared_ptr<MeasurementsEntryInfo>
  getMeasurementsEntryInfo(shared_ptr<measurements::Entry> entry);

  shared_ptr<MeasurementsEntryInfo>
  getMeasurementsEntryInfo(shared_ptr<pit::Entry> entry);

  /// propagate to another upstream
  void
  doPropagate(weak_ptr<pit::Entry> pitEntryWeak, weak_ptr<fib::Entry> fibEntryWeak);

  /// best face did not reply within prediction
  void
  timeoutOnBestFace(weak_ptr<pit::Entry> pitEntryWeak);

public:
  static const Name STRATEGY_NAME;

protected:
  static const time::microseconds DEFER_FIRST_WITHOUT_BEST_FACE;
  static const time::microseconds DEFER_RANGE_WITHOUT_BEST_FACE;
  static const int UPDATE_MEASUREMENTS_N_LEVELS = 2;
  static const time::nanoseconds MEASUREMENTS_LIFETIME;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_NCC_STRATEGY_HPP
