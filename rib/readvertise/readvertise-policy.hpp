/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_RIB_READVERTISE_READVERTISE_POLICY_HPP
#define NFD_RIB_READVERTISE_READVERTISE_POLICY_HPP

#include "../rib.hpp"
#include <ndn-cxx/security/signing-info.hpp>

namespace nfd {
namespace rib {

/** \brief a decision made by readvertise policy
 */
struct ReadvertiseAction
{
  Name prefix; ///< the prefix that should be readvertised
  ndn::security::SigningInfo signer; ///< credentials for command signing
};

/** \brief a policy to decide whether to readvertise a route, and what prefix to readvertise
 */
class ReadvertisePolicy : noncopyable
{
public:
  virtual
  ~ReadvertisePolicy() = default;

  /** \brief decide whether to readvertise a route, and what prefix to readvertise
   */
  virtual optional<ReadvertiseAction>
  handleNewRoute(const RibRouteRef& ribRoute) const = 0;

  /** \return how often readvertisements made by this policy should be refreshed.
   */
  virtual time::milliseconds
  getRefreshInterval() const = 0;
};

} // namespace rib
} // namespace nfd

#endif // NFD_RIB_READVERTISE_READVERTISE_POLICY_HPP
