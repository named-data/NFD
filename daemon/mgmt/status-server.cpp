/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "status-server.hpp"
#include "fw/forwarder.hpp"
#include "core/version.hpp"

namespace nfd {

const Name StatusServer::DATASET_PREFIX = "ndn:/localhost/nfd/status";
const time::milliseconds StatusServer::RESPONSE_FRESHNESS = time::milliseconds(5000);

StatusServer::StatusServer(shared_ptr<AppFace> face, Forwarder& forwarder)
  : m_face(face)
  , m_forwarder(forwarder)
  , m_startTimestamp(time::system_clock::now())
{
  m_face->setInterestFilter(DATASET_PREFIX, bind(&StatusServer::onInterest, this, _2));
}

void
StatusServer::onInterest(const Interest& interest) const
{
  Name name(DATASET_PREFIX);
  name.appendVersion();
  name.appendSegment(0);

  shared_ptr<Data> data = make_shared<Data>(name);
  data->setFreshnessPeriod(RESPONSE_FRESHNESS);

  shared_ptr<ndn::nfd::ForwarderStatus> status = this->collectStatus();
  data->setContent(status->wireEncode());

  m_face->sign(*data);
  m_face->put(*data);
}

shared_ptr<ndn::nfd::ForwarderStatus>
StatusServer::collectStatus() const
{
  shared_ptr<ndn::nfd::ForwarderStatus> status = make_shared<ndn::nfd::ForwarderStatus>();

  status->setNfdVersion(NFD_VERSION);
  status->setStartTimestamp(m_startTimestamp);
  status->setCurrentTimestamp(time::system_clock::now());

  status->setNNameTreeEntries(m_forwarder.getNameTree().size());
  status->setNFibEntries(m_forwarder.getFib().size());
  status->setNPitEntries(m_forwarder.getPit().size());
  status->setNMeasurementsEntries(m_forwarder.getMeasurements().size());
  status->setNCsEntries(m_forwarder.getCs().size());

  const ForwarderCounters& counters = m_forwarder.getCounters();
  status->setNInInterests(counters.getNInInterests());
  status->setNInDatas(counters.getNInDatas());
  status->setNOutInterests(counters.getNOutInterests());
  status->setNOutDatas(counters.getNOutDatas());

  return status;
}

} // namespace nfd
