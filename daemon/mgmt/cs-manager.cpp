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

#include "cs-manager.hpp"
#include <ndn-cxx/mgmt/nfd/cs-info.hpp>

namespace nfd {

CsManager::CsManager(Cs& cs, const ForwarderCounters& fwCnt,
                     Dispatcher& dispatcher, CommandAuthenticator& authenticator)
  : NfdManagerBase(dispatcher, authenticator, "cs")
  , m_fwCnt(fwCnt)
{
  registerStatusDatasetHandler("info", bind(&CsManager::serveInfo, this, _1, _2, _3));
}

void
CsManager::serveInfo(const Name& topPrefix, const Interest& interest,
                     ndn::mgmt::StatusDatasetContext& context) const
{
  ndn::nfd::CsInfo info;
  info.setNHits(m_fwCnt.nCsHits);
  info.setNMisses(m_fwCnt.nCsMisses);

  context.append(info.wireEncode());
  context.end();
}

} // namespace nfd
