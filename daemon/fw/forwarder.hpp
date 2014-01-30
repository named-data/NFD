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

namespace nfd {

/**
 * Forwarder is the main class of NFD.
 * 
 * It creates and owns a set of Face listeners
 */
class Forwarder
{
public:
  Forwarder(boost::asio::io_service& ioService);

  uint64_t
  addFace(const shared_ptr<Face>& face);

  void
  removeFace(const shared_ptr<Face>& face);

  void
  onInterest(const Face& face, const Interest& interest);

  void
  onData(const Face& face, const Data& data);
  
private: // pipelines
  /** \brief incoming Interest pipeline
   */
  void
  onIncomingInterest(Face& inFace, const Interest& interest);

  /** \brief Interest loop pipeline
   */
  void
  onInterestLoop(Face& inFace, const Interest& interest,
                 shared_ptr<pit::Entry> pitEntry);
  
  /** \brief outgoing Interest pipeline
   */
  void
  onOutgoingInterest(shared_ptr<pit::Entry> pitEntry, Face& outFace);
  
  /** \brief Interest rebuff pipeline
   */
  void
  onInterestRebuff(shared_ptr<pit::Entry> pitEntry);
  
  /** \brief Interest unsatisfied pipeline
   */
  void
  onInterestUnsatisfied(shared_ptr<pit::Entry> pitEntry);
  
  /** \brief incoming Data pipeline
   */
  void
  onIncomingData(Face& inFace, const Data& data);
  
  /** \brief Data unsolicited pipeline
   */
  void
  onDataUnsolicited(Face& inFace, const Data& data);
  
  /** \brief outgoing Data pipeline
   */
  void
  onOutgoingData(const Data& data, Face& outFace);

private:
  void
  setUnsatisfyTimer(shared_ptr<pit::Entry> pitEntry);
  
  void
  setStragglerTimer(shared_ptr<pit::Entry> pitEntry);
  
  void
  cancelUnsatisfyAndStragglerTimer(shared_ptr<pit::Entry> pitEntry);

private:
  Scheduler m_scheduler;
  Fib m_fib;
  Pit m_pit;
  Cs  m_cs;
  // container< shared_ptr<Face> > m_faces;
  
  // allow Strategy (base class) to enter pipelines
  friend class Strategy;
};

} // namespace nfd

#endif // NFD_FW_FORWARDER_HPP
