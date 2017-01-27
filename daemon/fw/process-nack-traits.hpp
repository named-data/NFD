/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_PROCESS_NACK_TRAITS_HPP
#define NFD_DAEMON_FW_PROCESS_NACK_TRAITS_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/** \brief provides a common procedure for processing Nacks
 *
 *  This procedure works as follows:
 *  1. If Nacks have been received from all upstream faces, return a Nack with least severe reason
 *     to downstream faces.
 *  2. If there are more than one upstream face, Nacks have been received from all but one of
 *     them, and that face is also a downstream, return a Nack to that face. This is to address a
 *     specific "live deadlock" scenario where two hosts are waiting for each other to return the
 *     Nack.
 *  3. Otherwise, wait for the arrival of more Nacks or Data.
 *
 *  To use this helper, the strategy should inherit from ProcessNackTraits<MyStrategy>,
 *  and declare that specialization as a friend class.
 *  Then, invoke processNack from afterReceiveNack trigger.
 */
class ProcessNackTraitsBase : noncopyable
{
protected:
  virtual
  ~ProcessNackTraitsBase() = default;

  void
  processNack(const Face& inFace, const lp::Nack& nack,
              const shared_ptr<pit::Entry>& pitEntry);

private:
  virtual void
  sendNackForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
                               const lp::NackHeader& header) = 0;

  virtual void
  sendNacksForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry,
                                const lp::NackHeader& header) = 0;
};

template<typename S>
class ProcessNackTraits : public ProcessNackTraitsBase
{
protected:
  explicit
  ProcessNackTraits(S* strategy)
    : m_strategy(strategy)
  {
  }

private:
  void
  sendNackForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry, const Face& outFace,
                               const lp::NackHeader& header) override
  {
    m_strategy->sendNack(pitEntry, outFace, header);
  }

  void
  sendNacksForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry,
                                const lp::NackHeader& header) override
  {
    m_strategy->sendNacks(pitEntry, header);
  }

private:
  S* m_strategy;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_PROCESS_NACK_TRAITS_HPP
