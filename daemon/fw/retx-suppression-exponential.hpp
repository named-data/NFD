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

#ifndef NFD_DAEMON_FW_RETX_SUPPRESSION_EXPONENTIAL_HPP
#define NFD_DAEMON_FW_RETX_SUPPRESSION_EXPONENTIAL_HPP

#include "retx-suppression.hpp"

namespace nfd {
namespace fw {

/** \brief a retransmission suppression decision algorithm that
 *         suppresses retransmissions using exponential backoff
 *
 *  The i-th retransmission will be suppressed if the last transmission (out-record)
 *  occurred within MIN(initialInterval * multiplier^(i-1), maxInterval)
 */
class RetxSuppressionExponential : public RetxSuppression
{
public:
  /** \brief time granularity
   */
  typedef time::microseconds Duration;

  explicit
  RetxSuppressionExponential(const Duration& initialInterval = DEFAULT_INITIAL_INTERVAL,
                             float multiplier = DEFAULT_MULTIPLIER,
                             const Duration& maxInterval = DEFAULT_MAX_INTERVAL);

  /** \brief determines whether Interest is a retransmission,
   *         and if so, whether it shall be forwarded or suppressed
   */
  virtual Result
  decide(const Face& inFace, const Interest& interest,
         pit::Entry& pitEntry) const DECL_OVERRIDE;

public:
  /** \brief StrategyInfo on pit::Entry
   */
  class PitInfo;

public:
  static const Duration DEFAULT_INITIAL_INTERVAL;
  static const float DEFAULT_MULTIPLIER;
  static const Duration DEFAULT_MAX_INTERVAL;

private:
  const Duration m_initialInterval;
  const float m_multiplier;
  const Duration m_maxInterval;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_RETX_SUPPRESSION_EXPONENTIAL_HPP
