/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_FW_FORWARDER_HPP
#define NFD_FW_FORWARDER_HPP

#include "common.hpp"
#include "core/scheduler.hpp"
#include "face/face.hpp"
#include "table/fib.hpp"
#include "table/pit.hpp"
#include "table/cs.hpp"
#include "table/measurements.hpp"
#include "strategy.hpp"

namespace nfd {

/**
 * Forwarder is the main class of NFD.
 *
 * It creates and owns a set of Face listeners
 */
class Forwarder
{
public:
  Forwarder();

  void
  addFace(shared_ptr<Face> face);

  void
  removeFace(shared_ptr<Face> face);

  void
  onInterest(Face& face, const Interest& interest);

  void
  onData(Face& face, const Data& data);

public:
  Fib&
  getFib();

  Pit&
  getPit();

  Cs&
  getCs();

  Measurements&
  getMeasurements();

  shared_ptr<Face>
  getFace(FaceId id);

PUBLIC_WITH_TESTS_ELSE_PRIVATE: // pipelines
  /** \brief incoming Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingInterest(Face& inFace, const Interest& interest);

  /** \brief Interest loop pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestLoop(Face& inFace, const Interest& interest,
                 shared_ptr<pit::Entry> pitEntry);

  /** \brief outgoing Interest pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingInterest(shared_ptr<pit::Entry> pitEntry, Face& outFace);

  /** \brief Interest reject pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestReject(shared_ptr<pit::Entry> pitEntry);

  /** \brief Interest unsatisfied pipeline
   */
  VIRTUAL_WITH_TESTS void
  onInterestUnsatisfied(shared_ptr<pit::Entry> pitEntry);

  /** \brief incoming Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onIncomingData(Face& inFace, const Data& data);

  /** \brief Data unsolicited pipeline
   */
  VIRTUAL_WITH_TESTS void
  onDataUnsolicited(Face& inFace, const Data& data);

  /** \brief outgoing Data pipeline
   */
  VIRTUAL_WITH_TESTS void
  onOutgoingData(const Data& data, Face& outFace);

PROTECTED_WITH_TESTS_ELSE_PRIVATE:
  VIRTUAL_WITH_TESTS void
  setUnsatisfyTimer(shared_ptr<pit::Entry> pitEntry);

  VIRTUAL_WITH_TESTS void
  setStragglerTimer(shared_ptr<pit::Entry> pitEntry);

  VIRTUAL_WITH_TESTS void
  cancelUnsatisfyAndStragglerTimer(shared_ptr<pit::Entry> pitEntry);

  VIRTUAL_WITH_TESTS void
  dispatchToStrategy(const Face& inFace,
                     const Interest& interest,
                     shared_ptr<fib::Entry> fibEntry,
                     shared_ptr<pit::Entry> pitEntry);

private:
  FaceId m_lastFaceId;
  std::map<FaceId, shared_ptr<Face> > m_faces;
  
  NameTree     m_nameTree;
  Fib          m_fib;
  Pit          m_pit;
  Cs           m_cs;
  Measurements m_measurements;
  /// the active strategy (only one strategy in mock)
  shared_ptr<fw::Strategy> m_strategy;

  static const Name s_localhostName;

  // allow Strategy (base class) to enter pipelines
  friend class fw::Strategy;
};

inline Fib&
Forwarder::getFib()
{
  return m_fib;
}

inline Pit&
Forwarder::getPit()
{
  return m_pit;
}

inline Cs&
Forwarder::getCs()
{
  return m_cs;
}

inline Measurements&
Forwarder::getMeasurements()
{
  return m_measurements;
}

} // namespace nfd

#endif // NFD_FW_FORWARDER_HPP
