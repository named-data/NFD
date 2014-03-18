/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_NCC_STRATEGY_HPP
#define NFD_FW_NCC_STRATEGY_HPP

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
                       shared_ptr<pit::Entry> pitEntry);

  virtual void
  beforeSatisfyPendingInterest(shared_ptr<pit::Entry> pitEntry,
                               const Face& inFace, const Data& data);

protected:
  /// StrategyInfo on measurements::Entry
  class MeasurementsEntryInfo : public StrategyInfo
  {
  public:
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
    weak_ptr<Face> m_bestFace;
    weak_ptr<Face> m_previousFace;
    time::nanoseconds m_prediction;

    static const time::nanoseconds INITIAL_PREDICTION;
    static const time::nanoseconds MIN_PREDICTION;
    static const int ADJUST_PREDICT_DOWN_SHIFT = 7;
    static const time::nanoseconds MAX_PREDICTION;
    static const int ADJUST_PREDICT_UP_SHIFT = 3;
  };

  /// StrategyInfo on pit::Entry
  class PitEntryInfo : public StrategyInfo
  {
  public:
    PitEntryInfo();

    virtual
    ~PitEntryInfo();

  public:
    bool m_isNewInterest;
    EventId m_bestFaceTimeout;
    EventId m_propagateTimer;
    time::nanoseconds m_maxInterval;
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
  static const time::nanoseconds DEFER_FIRST_WITHOUT_BEST_FACE;
  static const time::nanoseconds DEFER_RANGE_WITHOUT_BEST_FACE;
  static const int UPDATE_MEASUREMENTS_N_LEVELS = 2;
  static const time::nanoseconds MEASUREMENTS_LIFETIME;
};

} // namespace fw
} // namespace nfd

#endif // NFD_FW_NCC_STRATEGY_HPP
