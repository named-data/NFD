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

#ifndef NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP
#define NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP

#include "core/common.hpp"

namespace nfd {

/** \brief stores a collection of producer region names
 *
 *  This table is used in forwarding to process Interests with Link objects.
 *
 *  NetworkRegionTable exposes a set-like API, including methods `insert`, `clear`,
 *  `find`, `size`, `begin`, and `end`.
 */
class NetworkRegionTable : public std::set<Name>
{
public:
  /** \brief determines whether an Interest has reached a producer region
   *  \param forwardingHint forwarding hint of an Interest
   *  \retval true the Interest has reached a producer region
   *  \retval false the Interest has not reached a producer region
   *
   *  If any delegation name in the forwarding hint is a prefix of any region name,
   *  the Interest has reached the producer region and should be forwarded according to ‎its Name;
   *  otherwise, the Interest should be forwarded according to the forwarding hint.
   */
  bool
  isInProducerRegion(const DelegationList& forwardingHint) const;
};

} // namespace nfd

#endif // NFD_DAEMON_TABLE_NETWORK_REGION_TABLE_HPP
