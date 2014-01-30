/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "forwarder.hpp"

namespace nfd {

Forwarder::Forwarder(boost::asio::io_service& ioService)
  : m_scheduler(ioService)
{
}

uint64_t
Forwarder::addFace(const shared_ptr<Face>& face)
{
  return -1;
}

void
Forwarder::removeFace(const shared_ptr<Face>& face)
{
}

void
Forwarder::onInterest(const Face& face, const Interest& interest)
{
  this->onIncomingInterest(const_cast<Face&>(face), interest);
}

void
Forwarder::onData(const Face& face, const Data& data)
{
  this->onIncomingData(const_cast<Face&>(face), data);
}

void
Forwarder::onIncomingInterest(Face& inFace, const Interest& interest)
{
  // receive Interest
  
  // PIT insert
  std::pair<shared_ptr<pit::Entry>, bool>
    pitInsertResult = m_pit.insert(interest);
  shared_ptr<pit::Entry> pitEntry = pitInsertResult.first;
  
  // detect loop and record Nonce
  bool isLoop = ! pitEntry->addNonce(interest.getNonce());
  if (isLoop) {
    // goto Interest loop pipeline
    this->onInterestLoop(inFace, interest, pitEntry);
    return;
  }
  
  // cancel unsatisfy & straggler timer
  this->cancelUnsatisfyAndStragglerTimer(pitEntry);
  
  const pit::InRecordCollection& inRecords = pitEntry->getInRecords();
  bool isPending = inRecords.begin() == inRecords.end();
  
  if (!isPending) {
    // CS lookup
    const Data* csMatch = m_cs.find(interest);
    if (csMatch != 0) {
      // XXX should we lookup PIT for other Interests that also match csMatch?

      // goto outgoing Data pipeline
      this->onOutgoingData(*csMatch, inFace);
      return;
    }
  }
  
  // insert InRecord
  pit::InRecordCollection::iterator inRecordIt =
    pitEntry->insertOrUpdateInRecord(inFace.shared_from_this(), interest);
  
  // app chosen nexthops
  bool isAppChosenNexthops = false; // TODO get from local control header
  if (isAppChosenNexthops) {
    // TODO foreach chosen nexthop: goto outgoing Interest pipeline
    return;
  }
  
  // FIB lookup
  shared_ptr<fib::Entry> fibEntry
    = m_fib.findLongestPrefixMatch(interest.getName());
  // TODO use Fib::findParent(pitEntry)
  
  // dispatch to strategy
  // TODO
}

void
Forwarder::onInterestLoop(Face& inFace, const Interest& interest,
                          shared_ptr<pit::Entry> pitEntry)
{
  // do nothing, which means Interest is dropped
}

void
Forwarder::onOutgoingInterest(shared_ptr<pit::Entry> pitEntry, Face& outFace)
{
  // pick Interest
  const Interest& interest = pitEntry->getInterest();
  // TODO pick the last incoming Interest
  
  // insert OutRecord
  pit::OutRecordCollection::iterator outRecordIt =
    pitEntry->insertOrUpdateOutRecord(outFace.shared_from_this(), interest);
  
  // set PIT unsatisfy timer
  this->setUnsatisfyTimer(pitEntry);
  
  // send Interest
  outFace.sendInterest(interest);
}

void
Forwarder::onInterestRebuff(shared_ptr<pit::Entry> pitEntry)
{
  // set PIT straggler timer
  this->setStragglerTimer(pitEntry);
}

void
Forwarder::onInterestUnsatisfied(shared_ptr<pit::Entry> pitEntry)
{
  // invoke PIT unsatisfied callback
  // TODO
  
  // PIT delete
  m_pit.remove(pitEntry);
}

void
Forwarder::onIncomingData(Face& inFace, const Data& data)
{
  // receive Data
  
  // PIT match
  shared_ptr<pit::DataMatchResult> pitMatches = m_pit.findAllDataMatches(data);
  if (pitMatches->begin() == pitMatches->end()) {
    // goto Data unsolicited pipeline
    this->onDataUnsolicited(inFace, data);
    return;
  }
  
  // CS insert
  m_cs.insert(data);
  
  std::set<shared_ptr<Face> > pendingDownstreams;
  // foreach PitEntry
  for (pit::DataMatchResult::iterator it = pitMatches->begin();
       it != pitMatches->end(); ++it) {
    shared_ptr<pit::Entry> pitEntry = *it;
    
    // cancel unsatisfy & straggler timer
    this->cancelUnsatisfyAndStragglerTimer(pitEntry);
    
    // remember pending downstreams
    const pit::InRecordCollection& inRecords = pitEntry->getInRecords();
    for (pit::InRecordCollection::const_iterator it = inRecords.begin();
                                                 it != inRecords.end(); ++it) {
      if (it->getExpiry() > time::now()) {
        pendingDownstreams.insert(it->getFace());
      }
    }
    
    // mark PIT satisfied
    pitEntry->deleteInRecords();
    pitEntry->deleteOutRecord(inFace.shared_from_this());
    
    // set PIT straggler timer
    this->setStragglerTimer(pitEntry);
    
    // invoke PIT satisfy callback
    // TODO
  }
  
  // foreach pending downstream
  for (std::set<shared_ptr<Face> >::iterator it = pendingDownstreams.begin();
      it != pendingDownstreams.end(); ++it) {
    // goto outgoing Data pipeline
    this->onOutgoingData(data, **it);
  }
}

void
Forwarder::onDataUnsolicited(Face& inFace, const Data& data)
{
  // accept to cache?
  bool acceptToCache = false;// TODO decision
  if (acceptToCache) {
    // CS insert
    m_cs.insert(data);
  }
}

void
Forwarder::onOutgoingData(const Data& data, Face& outFace)
{
  // traffic manager
  // pass through
  
  // send Data
  outFace.sendData(data);
}

static inline bool
compare_InRecord_expiry(const pit::InRecord& a, const pit::InRecord& b)
{
  return a.getExpiry() < b.getExpiry();
}

void
Forwarder::setUnsatisfyTimer(shared_ptr<pit::Entry> pitEntry)
{
  const pit::InRecordCollection& inRecords = pitEntry->getInRecords();
  pit::InRecordCollection::const_iterator lastExpiring =
    std::max_element(inRecords.begin(), inRecords.end(),
    &compare_InRecord_expiry);

  time::Point lastExpiry = lastExpiring->getExpiry();
  time::Duration lastExpiryFromNow = lastExpiry  - time::now();
  if (lastExpiryFromNow <= time::seconds(0)) {
    // TODO all InRecords are already expired; will this happen?
  }
  
  pitEntry->m_unsatisfyTimer = m_scheduler.scheduleEvent(lastExpiryFromNow,
    bind(&Forwarder::onInterestUnsatisfied, this, pitEntry));
}

void
Forwarder::setStragglerTimer(shared_ptr<pit::Entry> pitEntry)
{
  time::Duration stragglerTime = time::milliseconds(100);
  
  pitEntry->m_stragglerTimer = m_scheduler.scheduleEvent(stragglerTime,
    bind(&Pit::remove, &m_pit, pitEntry));
}

void
Forwarder::cancelUnsatisfyAndStragglerTimer(shared_ptr<pit::Entry> pitEntry)
{
  m_scheduler.cancelEvent(pitEntry->m_unsatisfyTimer);
  m_scheduler.cancelEvent(pitEntry->m_stragglerTimer);
}



} // namespace nfd
