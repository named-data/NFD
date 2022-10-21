/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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
#include "strategy.hpp"

namespace nfd::fw {

/**
 * \brief A retransmission suppression decision algorithm that suppresses
 *        retransmissions using exponential backoff.
 *
 * The i-th retransmission will be suppressed if the last transmission (out-record)
 * occurred within `MIN(initialInterval * multiplier^(i-1), maxInterval)`.
 */
class RetxSuppressionExponential
{
public:
  /**
   * \brief Time granularity.
   */
  using Duration = time::milliseconds;

  explicit
  RetxSuppressionExponential(Duration initialInterval = DEFAULT_INITIAL_INTERVAL,
                             Duration maxInterval = DEFAULT_MAX_INTERVAL,
                             float multiplier = DEFAULT_MULTIPLIER);

  /** \brief Determines whether Interest is a retransmission per pit entry
   *         and if so, whether it shall be forwarded or suppressed.
   */
  RetxSuppressionResult
  decidePerPitEntry(pit::Entry& pitEntry);

  /** \brief Determines whether Interest is a retransmission per upstream
   *         and if so, whether it shall be forwarded or suppressed.
   */
  RetxSuppressionResult
  decidePerUpstream(pit::Entry& pitEntry, Face& outFace);

  /** \brief Increment the suppression interval for out record.
   */
  void
  incrementIntervalForOutRecord(pit::OutRecord& outRecord);

  static std::unique_ptr<RetxSuppressionExponential>
  construct(const StrategyParameters& params);

private: // non-member operators (hidden friends)
  friend std::ostream&
  operator<<(std::ostream& os, const RetxSuppressionExponential& retxSupp)
  {
    return os << "RetxSuppressionExponential initial-interval=" << retxSupp.m_initialInterval
              << " max-interval=" << retxSupp.m_maxInterval
              << " multiplier=" << retxSupp.m_multiplier;
  }

public:
  static constexpr Duration DEFAULT_INITIAL_INTERVAL = 10_ms;
  static constexpr Duration DEFAULT_MAX_INTERVAL = 250_ms;
  static constexpr float DEFAULT_MULTIPLIER = 2.0f;

NFD_PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  const Duration m_initialInterval;
  const Duration m_maxInterval;
  const float m_multiplier;
};

} // namespace nfd::fw

#endif // NFD_DAEMON_FW_RETX_SUPPRESSION_EXPONENTIAL_HPP
