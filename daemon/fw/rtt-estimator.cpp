/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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

#include "rtt-estimator.hpp"

namespace nfd {

RttEstimator::RttEstimator(uint16_t maxMultiplier, Duration minRto, double gain)
  : m_maxMultiplier(maxMultiplier)
  , m_minRto(minRto.count())
  , m_rtt(RttEstimator::getInitialRtt().count())
  , m_gain(gain)
  , m_variance(0)
  , m_multiplier(1)
  , m_nSamples(0)
{
}

void
RttEstimator::addMeasurement(Duration measure)
{
  double m = static_cast<double>(measure.count());
  if (m_nSamples > 0) {
    double err = m - m_rtt;
    double gErr = err * m_gain;
    m_rtt += gErr;
    double difference = std::abs(err) - m_variance;
    m_variance += difference * m_gain;
  } else {
    m_rtt = m;
    m_variance = m;
  }
  ++m_nSamples;
  m_multiplier = 1;
}

void
RttEstimator::incrementMultiplier()
{
  m_multiplier = std::min(static_cast<uint16_t>(m_multiplier + 1), m_maxMultiplier);
}

void
RttEstimator::doubleMultiplier()
{
  m_multiplier = std::min(static_cast<uint16_t>(m_multiplier * 2), m_maxMultiplier);
}

RttEstimator::Duration
RttEstimator::computeRto() const
{
  double rto = std::max(m_minRto, m_rtt + 4 * m_variance);
  rto *= m_multiplier;
  return Duration(static_cast<Duration::rep>(rto));
}

} // namespace nfd
