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

#ifndef NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP
#define NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP

#include "readvertise-destination.hpp"
#include "rib/rib.hpp"

#include <ndn-cxx/mgmt/nfd/command-options.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/control-parameters.hpp>

namespace nfd {
namespace rib {

/** \brief a readvertise destination using NFD RIB management protocol
 */
class NfdRibReadvertiseDestination : public ReadvertiseDestination
{
public:
  NfdRibReadvertiseDestination(ndn::nfd::Controller& controller,
                               Rib& rib,
                               const ndn::nfd::CommandOptions& options = ndn::nfd::CommandOptions(),
                               const ndn::nfd::ControlParameters& parameters =
                                 ndn::nfd::ControlParameters()
                                   .setOrigin(ndn::nfd::ROUTE_ORIGIN_CLIENT));

  /** \brief add a name prefix into NFD RIB
   */
  void
  advertise(const ReadvertisedRoute& rr,
            std::function<void()> successCb,
            std::function<void(const std::string&)> failureCb) override;

  /** \brief remove a name prefix from NFD RIB
   */
  void
  withdraw(const ReadvertisedRoute& rr,
           std::function<void()> successCb,
           std::function<void(const std::string&)> failureCb) override;

protected:
  ndn::nfd::ControlParameters
  getControlParameters();

  ndn::nfd::CommandOptions
  getCommandOptions();

private:
  void
  handleRibInsert(const Name& name);

  void
  handleRibErase(const Name& name);

private:
  ndn::nfd::Controller& m_controller;

  signal::ScopedConnection m_ribInsertConn;
  signal::ScopedConnection m_ribEraseConn;

  ndn::nfd::CommandOptions m_commandOptions;
  ndn::nfd::ControlParameters m_controlParameters;
};

} // namespace rib
} // namespace nfd

#endif // NFD_DAEMON_RIB_READVERTISE_NFD_RIB_READVERTISE_DESTINATION_HPP
