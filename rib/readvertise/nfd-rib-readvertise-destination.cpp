/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
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

#include "nfd-rib-readvertise-destination.hpp"

namespace nfd {
namespace rib {

using ndn::nfd::ControlParameters;
using ndn::nfd::ControlResponse;

NfdRibReadvertiseDestination::NfdRibReadvertiseDestination(ndn::nfd::Controller& controller,
                                                           const ndn::Name& commandPrefix,
                                                           Rib& rib)
  : m_controller(controller)
  , m_commandPrefix(commandPrefix)
{
  m_ribAddConn = rib.afterInsertEntry.connect(
    std::bind(&NfdRibReadvertiseDestination::handleRibAdd, this, _1));
  m_ribRemoveConn = rib.afterEraseEntry.connect(
    std::bind(&NfdRibReadvertiseDestination::handleRibRemove, this, _1));
}

void
NfdRibReadvertiseDestination::advertise(nfd::rib::ReadvertisedRoute& rr,
                                        std::function<void()> successCb,
                                        std::function<void(const std::string&)> failureCb)
{
  m_controller.start<ndn::nfd::RibRegisterCommand>(ControlParameters()
                                                   .setName(rr.getPrefix())
                                                   .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT),
    [=] (const ControlParameters& cp) { successCb(); },
    [=] (const ControlResponse& cr) { failureCb(cr.getText()); });
}

void
NfdRibReadvertiseDestination::withdraw(nfd::rib::ReadvertisedRoute& rr,
                                       std::function<void()> successCb,
                                       std::function<void(const std::string&)> failureCb)
{
  m_controller.start<ndn::nfd::RibUnregisterCommand>(ControlParameters()
                                                     .setName(rr.getPrefix())
                                                     .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT),
    [=] (const ControlParameters& cp) { successCb(); },
    [=] (const ControlResponse& cr) { failureCb(cr.getText()); });
}

void
NfdRibReadvertiseDestination::handleRibAdd(const ndn::Name& name)
{
  if (name == m_commandPrefix) {
    setAvailability(true);
  }
}

void
NfdRibReadvertiseDestination::handleRibRemove(const ndn::Name& name)
{
  if (name == m_commandPrefix) {
    setAvailability(false);
  }
}

} // namespace rib
} // namespace nfd
