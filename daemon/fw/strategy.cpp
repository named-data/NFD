/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy.hpp"
#include "forwarder.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace fw {

NFD_LOG_INIT("Strategy");

Strategy::Strategy(Forwarder& forwarder)
  : m_forwarder(forwarder)
{
}

Strategy::~Strategy()
{
}

void
Strategy::beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  NFD_LOG_DEBUG("beforeExpirePendingInterest pitEntry=" << pitEntry->getName());
}

void
Strategy::afterAddFibEntry(shared_ptr<fib::Entry> fibEntry)
{
  NFD_LOG_DEBUG("afterAddFibEntry fibEntry=" << fibEntry->getPrefix());
}

void
Strategy::afterUpdateFibEntry(shared_ptr<fib::Entry> fibEntry)
{
  NFD_LOG_DEBUG("afterUpdateFibEntry fibEntry=" << fibEntry->getPrefix());
}

void
Strategy::beforeRemoveFibEntry(shared_ptr<fib::Entry> fibEntry)
{
  NFD_LOG_DEBUG("beforeRemoveFibEntry fibEntry=" << fibEntry->getPrefix());
}

void
Strategy::sendInterest(shared_ptr<pit::Entry> pitEntry,
                       shared_ptr<Face> outFace)
{
  m_forwarder.onOutgoingInterest(pitEntry, *outFace);
}

void
Strategy::rebuffPendingInterest(shared_ptr<pit::Entry> pitEntry)
{
  m_forwarder.onInterestRebuff(pitEntry);
}

} // namespace fw
} // namespace nfd
