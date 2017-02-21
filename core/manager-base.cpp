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

#include "manager-base.hpp"

namespace nfd {

using ndn::mgmt::ValidateParameters;
using ndn::mgmt::Authorization;

ManagerBase::ManagerBase(Dispatcher& dispatcher, const std::string& module)
  : m_dispatcher(dispatcher)
  , m_module(module)
{
}

ManagerBase::~ManagerBase() = default;

void
ManagerBase::registerStatusDatasetHandler(const std::string& verb,
                                          const ndn::mgmt::StatusDatasetHandler& handler)
{
  m_dispatcher.addStatusDataset(makeRelPrefix(verb),
                                ndn::mgmt::makeAcceptAllAuthorization(),
                                handler);
}

ndn::mgmt::PostNotification
ManagerBase::registerNotificationStream(const std::string& verb)
{
  return m_dispatcher.addNotificationStream(makeRelPrefix(verb));
}

void
ManagerBase::extractRequester(const Interest& interest,
                              ndn::mgmt::AcceptContinuation accept)
{
  const Name& interestName = interest.getName();

  try {
    ndn::SignatureInfo sigInfo(interestName.at(ndn::signed_interest::POS_SIG_INFO).blockFromValue());
    if (!sigInfo.hasKeyLocator() ||
        sigInfo.getKeyLocator().getType() != ndn::KeyLocator::KeyLocator_Name) {
      return accept("");
    }

    accept(sigInfo.getKeyLocator().getName().toUri());
  }
  catch (const tlv::Error&) {
    accept("");
  }
}

bool
ManagerBase::validateParameters(const nfd::ControlCommand& command, const ndn::mgmt::ControlParameters& parameters)
{
  BOOST_ASSERT(dynamic_cast<const ControlParameters*>(&parameters) != nullptr);

  try {
    command.validateRequest(static_cast<const ControlParameters&>(parameters));
  }
  catch (const ControlCommand::ArgumentError&) {
    return false;
  }
  return true;
}

void
ManagerBase::handleCommand(shared_ptr<nfd::ControlCommand> command,
                           const ControlCommandHandler& handler,
                           const Name& prefix, const Interest& interest,
                           const ndn::mgmt::ControlParameters& params,
                           ndn::mgmt::CommandContinuation done)
{
  BOOST_ASSERT(dynamic_cast<const ControlParameters*>(&params) != nullptr);
  ControlParameters parameters = static_cast<const ControlParameters&>(params);
  command->applyDefaultsToRequest(parameters);
  handler(*command, prefix, interest, parameters, done);
}

} // namespace nfd
