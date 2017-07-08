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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP

#include "stage.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {

/** \brief multicast discovery stage
 *
 *  This stage locates an NDN gateway router, commonly known as a "hub", in the local network by
 *  sending a hub discovery Interest ndn:/localhop/ndn-autoconf/hub via multicast. This class
 *  configures routes and strategy on local NFD, so that this Interest is multicast to all
 *  multi-access faces.
 *
 *  If an NDN gateway router is present in the local network, it should reply with a Data
 *  containing its own FaceUri. The Data payload contains a Uri element, and the value of this
 *  element is an ASCII-encoded string of the router's FaceUri. The router may use
 *  ndn-autoconfig-server program to serve this Data.
 *
 *  Signature on this Data is currently not verified. This stage succeeds when the Data is
 *  successfully decoded.
 */
class MulticastDiscovery : public Stage
{
public:
  MulticastDiscovery(Face& face, nfd::Controller& controller);

  const std::string&
  getName() const final
  {
    static const std::string STAGE_NAME("multicast discovery");
    return STAGE_NAME;
  }

private:
  void
  doStart() final;

  void
  registerHubDiscoveryPrefix(const std::vector<nfd::FaceStatus>& dataset);

  void
  afterReg();

  void
  setStrategy();

  void
  requestHubData();

private:
  Face& m_face;
  nfd::Controller& m_controller;

  int m_nRegs = 0;
  int m_nRegSuccess = 0;
  int m_nRegFailure = 0;
};

} // namespace autoconfig
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_MULTICAST_DISCOVERY_HPP
