/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#include "congestion-mark-strategy.hpp"

#include <ndn-cxx/lp/tags.hpp>

namespace nfd {
namespace fw {

NFD_LOG_INIT("CongestionMarkStrategy");
NFD_REGISTER_STRATEGY(CongestionMarkStrategy);

CongestionMarkStrategy::CongestionMarkStrategy(Forwarder& forwarder, const Name& name)
  : BestRouteStrategy2(forwarder, name)
  , m_congestionMark(1)
  , m_shouldPreserveMark(true)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  switch (parsed.parameters.size()) {
  case 2:
    if (!parsed.parameters.at(1).isNumber()) {
      BOOST_THROW_EXCEPTION(
        std::invalid_argument("Second parameter to CongestionMarkStrategy must be a non-negative integer"));
    }
    m_shouldPreserveMark = parsed.parameters.at(1).toNumber() != 0;
    NDN_CXX_FALLTHROUGH;
  case 1:
    if (!parsed.parameters.at(0).isNumber()) {
      BOOST_THROW_EXCEPTION(
        std::invalid_argument("First parameter to CongestionMarkStrategy must be a non-negative integer"));
    }
    m_congestionMark = parsed.parameters.at(0).toNumber();
    NDN_CXX_FALLTHROUGH;
  case 0:
    break;
  default:
    BOOST_THROW_EXCEPTION(std::invalid_argument("CongestionMarkStrategy does not accept more than 2 parameters"));
  }

  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    BOOST_THROW_EXCEPTION(std::invalid_argument(
      "CongestionMarkStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name&
CongestionMarkStrategy::getStrategyName()
{
  static Name strategyName("/localhost/nfd/strategy/congestion-mark/%FD%01");
  return strategyName;
}

void
CongestionMarkStrategy::afterReceiveInterest(const Face& inFace, const Interest& interest,
                                             const shared_ptr<pit::Entry>& pitEntry)
{
  auto mark = interest.getCongestionMark();
  if (mark != m_congestionMark && (!m_shouldPreserveMark || mark == 0)) {
    Interest markedInterest(interest);
    markedInterest.setCongestionMark(m_congestionMark);
    BestRouteStrategy2::afterReceiveInterest(inFace, markedInterest, pitEntry);
  }
  else {
    BestRouteStrategy2::afterReceiveInterest(inFace, interest, pitEntry);
  }
}

} // namespace fw
} // namespace nfd
