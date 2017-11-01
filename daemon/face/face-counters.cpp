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

#include "face-counters.hpp"

namespace nfd {
namespace face {

FaceCounters::FaceCounters(const LinkService::Counters& linkServiceCounters,
                           const Transport::Counters& transportCounters)
  : nInInterests(linkServiceCounters.nInInterests)
  , nOutInterests(linkServiceCounters.nOutInterests)
  , nDroppedInterests(linkServiceCounters.nDroppedInterests)
  , nInData(linkServiceCounters.nInData)
  , nOutData(linkServiceCounters.nOutData)
  , nInNacks(linkServiceCounters.nInNacks)
  , nOutNacks(linkServiceCounters.nOutNacks)
  , nInPackets(transportCounters.nInPackets)
  , nOutPackets(transportCounters.nOutPackets)
  , nInBytes(transportCounters.nInBytes)
  , nOutBytes(transportCounters.nOutBytes)
  , m_linkServiceCounters(linkServiceCounters)
  , m_transportCounters(transportCounters)
{
}

} // namespace face
} // namespace nfd
