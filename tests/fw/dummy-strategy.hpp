/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_TEST_FW_DUMMY_STRATEGY_HPP
#define NFD_TEST_FW_DUMMY_STRATEGY_HPP

#include "fw/strategy.hpp"

namespace nfd {
namespace tests {

/** \brief strategy for unit testing
 *
 *  Triggers on DummyStrategy are recorded but does nothing
 */
class DummyStrategy : public fw::Strategy
{
public:
  DummyStrategy(Forwarder& forwarder, const Name& name)
    : Strategy(forwarder, name)
  {
  }

  virtual void
  afterReceiveInterest(const Face& inFace,
                       const Interest& interest,
                       shared_ptr<fib::Entry> fibEntry,
                       shared_ptr<pit::Entry> pitEntry)
  {
    ++m_afterReceiveInterest_count;

    if (static_cast<bool>(m_interestOutFace)) {
      this->sendInterest(pitEntry, m_interestOutFace);
    }
    else {
      this->rejectPendingInterest(pitEntry);
    }
  }

  virtual void
  beforeSatisfyPendingInterest(shared_ptr<pit::Entry> pitEntry,
                               const Face& inFace, const Data& data)
  {
    ++m_beforeSatisfyPendingInterest_count;
  }

  virtual void
  beforeExpirePendingInterest(shared_ptr<pit::Entry> pitEntry)
  {
    ++m_beforeExpirePendingInterest_count;
  }

public:
  int m_afterReceiveInterest_count;
  int m_beforeSatisfyPendingInterest_count;
  int m_beforeExpirePendingInterest_count;

  /// outFace to use in afterReceiveInterest, nullptr to reject
  shared_ptr<Face> m_interestOutFace;
};

} // namespace tests
} // namespace nfd

#endif // TEST_FW_DUMMY_STRATEGY_HPP
