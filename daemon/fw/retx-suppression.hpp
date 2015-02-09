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

#ifndef NFD_DAEMON_FW_RETX_SUPPRESSION_HPP
#define NFD_DAEMON_FW_RETX_SUPPRESSION_HPP

#include "strategy.hpp"

namespace nfd {
namespace fw {

/** \brief helper for consumer retransmission suppression
 */
class RetxSuppression : noncopyable
{
public:
  enum Result {
    /** \brief Interest is new (not a retransmission)
     */
    NEW,

    /** \brief Interest is retransmission and should be forwarded
     */
    FORWARD,

    /** \brief Interest is retransmission and should be suppressed
     */
    SUPPRESS
  };

  /** \brief determines whether Interest is a retransmission,
   *         and if so, whether it shall be forwarded or suppressed
   */
  virtual Result
  decide(const Face& inFace, const Interest& interest, pit::Entry& pitEntry) const = 0;

protected:
  /** \return last out-record time
   *  \pre pitEntry has one or more unexpired out-records
   */
  time::steady_clock::TimePoint
  getLastOutgoing(const pit::Entry& pitEntry) const;
};

} // namespace fw
} // namespace nfd

#endif // NFD_DAEMON_FW_RETX_SUPPRESSION_HPP
