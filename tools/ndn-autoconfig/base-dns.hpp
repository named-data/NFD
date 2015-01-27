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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_BASE_DNS_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_BASE_DNS_HPP

#include "base.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

/**
 * @brief Base class for stages that use DNS-based guessing
 */
class BaseDns : public Base
{
protected:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  BaseDns(Face& face, KeyChain& keyChain, const NextStageCallback& nextStageOnFailure);

  /**
   * @brief Send DNS SRV request for a @p fqdn fully qualified domain name
   * @return FaceUri of the hub from the requested SRV record
   * @throw Error if query returns nothing or SRV record cannot be parsed
   */
  std::string
  querySrvRr(const std::string& fqdn);

  /**
   * @brief Send DNS SRV request using search domain list
   * @return FaceUri of the hub from the requested SRV record
   * @throw Error if query returns nothing or SRV record cannot be parsed
   */
  std::string
  querySrvRrSearch();

private:
  union QueryAnswer;

  /**
   * @brief Parse SRV record
   * @return FaceUri of the hub from the SRV record
   * @throw Error if SRV record cannot be parsed
   */
  std::string
  parseSrvRr(const QueryAnswer& queryAnswer, int answerSize);
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_BASE_DNS_HPP
