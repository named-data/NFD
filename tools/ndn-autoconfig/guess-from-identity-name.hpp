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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_GUESS_FROM_IDENTITY_NAME_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_GUESS_FROM_IDENTITY_NAME_HPP

#include "base-dns.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

/**
 * @brief Guessing home router based on the default identity name
 *
 * This stage assumes that user has configured default certificate using
 * http://ndncert.named-data.net/
 *
 * - Request
 *
 *     The end host loads the default user identity (eg. /ndn/edu/ucla/cs/afanasev), and
 *     converts it to DNS format.
 *
 *     The end host sends a DNS query for an SRV record of name _ndn._udp. + user identity in
 *     DNS format + _homehub._auto-conf.named-data.net. For example:
 *
 *         _ndn._udp.afanasev.cs.ucla.edu.ndn._homehub._autoconf.named-data.net
 *
 * - Response
 *
 *     The DNS server should answer with an SRV record that contains the hostname and UDP port
 *     number of the home NDN router of this user's site.
 */
class GuessFromIdentityName : public BaseDns
{
public:
  /**
   * @brief Create stage to guess home router based on the default identity name
   * @sa Base::Base
   */
  GuessFromIdentityName(Face& face, KeyChain& keyChain,
                        const NextStageCallback& nextStageOnFailure);

  virtual void
  start() DECL_OVERRIDE;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_GUESSING_FROM_IDENTITY_NAME_HPP
