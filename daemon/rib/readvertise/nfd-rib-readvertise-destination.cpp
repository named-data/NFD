/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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
#include "common/logger.hpp"

#include <ndn-cxx/mgmt/nfd/control-command.hpp>
#include <ndn-cxx/mgmt/nfd/control-response.hpp>

namespace nfd {
namespace rib {

NFD_LOG_INIT(NfdRibReadvertiseDestination);

using ndn::nfd::ControlResponse;

NfdRibReadvertiseDestination::NfdRibReadvertiseDestination(ndn::nfd::Controller& controller,
                                                           Rib& rib,
                                                           const ndn::nfd::CommandOptions& options,
                                                           const ndn::nfd::ControlParameters& parameters)
  : m_controller(controller)
  , m_commandOptions(options)
  , m_controlParameters(parameters)
{
  m_ribInsertConn = rib.afterInsertEntry.connect(
    std::bind(&NfdRibReadvertiseDestination::handleRibInsert, this, _1));
  m_ribEraseConn = rib.afterEraseEntry.connect(
    std::bind(&NfdRibReadvertiseDestination::handleRibErase, this, _1));
}

void
NfdRibReadvertiseDestination::advertise(const nfd::rib::ReadvertisedRoute& rr,
                                        std::function<void()> successCb,
                                        std::function<void(const std::string&)> failureCb)
{
  NFD_LOG_DEBUG("advertise " << rr.prefix << " on " << m_commandOptions.getPrefix());

  m_controller.start<ndn::nfd::RibRegisterCommand>(
    getControlParameters().setName(rr.prefix),
    [=] (const ControlParameters& cp) { successCb(); },
    [=] (const ControlResponse& cr) { failureCb(cr.getText()); },
    getCommandOptions().setSigningInfo(rr.signer));
}

void
NfdRibReadvertiseDestination::withdraw(const nfd::rib::ReadvertisedRoute& rr,
                                       std::function<void()> successCb,
                                       std::function<void(const std::string&)> failureCb)
{
  NFD_LOG_DEBUG("withdraw " << rr.prefix << " on " << m_commandOptions.getPrefix());

  m_controller.start<ndn::nfd::RibUnregisterCommand>(
    getControlParameters().setName(rr.prefix),
    [=] (const ControlParameters& cp) { successCb(); },
    [=] (const ControlResponse& cr) { failureCb(cr.getText()); },
    getCommandOptions().setSigningInfo(rr.signer));
}

ndn::nfd::ControlParameters
NfdRibReadvertiseDestination::getControlParameters()
{
  return m_controlParameters;
}

ndn::nfd::CommandOptions
NfdRibReadvertiseDestination::getCommandOptions()
{
  return m_commandOptions;
}

void
NfdRibReadvertiseDestination::handleRibInsert(const ndn::Name& name)
{
  if (name.isPrefixOf(m_commandOptions.getPrefix())) {
    setAvailability(true);
  }
}

void
NfdRibReadvertiseDestination::handleRibErase(const ndn::Name& name)
{
  if (name.isPrefixOf(m_commandOptions.getPrefix())) {
    setAvailability(false);
  }
}

} // namespace rib
} // namespace nfd
