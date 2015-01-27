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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP

#include "base.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

/**
 * @brief Multicast discovery stage
 *
 * - Request
 *
 *     The end host sends an Interest over a multicast face.
 *
 *     Interest Name is /localhop/ndn-autoconf/hub.
 *
 * - Response
 *
 *     A producer app on the HUB answer this Interest with a Data packet that contains a
 *     TLV-encoded Uri block.  The value of this block is the URI for the HUB, preferably a
 *     UDP tunnel.
 */
class MulticastDiscovery : public Base
{
public:
  /**
   * @brief Create multicast discovery stage
   * @sa Base::Base
   */
  MulticastDiscovery(Face& face, KeyChain& keyChain, const NextStageCallback& nextStageOnFailure);

  virtual void
  start() DECL_OVERRIDE;

private:
  void
  registerHubDiscoveryPrefix(const ConstBufferPtr& buffer);

  void
  onRegisterSuccess();

  void
  onRegisterFailure(uint32_t code, const std::string& error);

  void
  setStrategy();

  void
  onSetStrategyFailure(const std::string& error);

  // Start to look for a hub (NDN hub discovery first stage)
  void
  requestHubData();

  void
  onSuccess(Data& data);

private:
  size_t nRequestedRegs;
  size_t nFinishedRegs;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP
