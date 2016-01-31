/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include "nfd-manager-base.hpp"

namespace nfd {

NfdManagerBase::NfdManagerBase(Dispatcher& dispatcher,
                               CommandValidator& validator,
                               const std::string& module)
  : ManagerBase(dispatcher, module)
  , m_validator(validator)
{
  m_validator.addSupportedPrivilege(module);
}

void
NfdManagerBase::authorize(const Name& prefix, const Interest& interest,
                          const ndn::mgmt::ControlParameters* params,
                          ndn::mgmt::AcceptContinuation accept,
                          ndn::mgmt::RejectContinuation reject)
{
  BOOST_ASSERT(params != nullptr);
  BOOST_ASSERT(typeid(*params) == typeid(ndn::nfd::ControlParameters));

  m_validator.validate(interest,
                       bind([&interest, this, accept] { extractRequester(interest, accept); }),
                       bind([reject] { reject(ndn::mgmt::RejectReply::STATUS403); }));
}

} // namespace nfd
