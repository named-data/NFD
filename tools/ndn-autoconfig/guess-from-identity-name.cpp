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

#include "guess-from-identity-name.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

GuessFromIdentityName::GuessFromIdentityName(Face& face, KeyChain& keyChain,
                                             const NextStageCallback& nextStageOnFailure)
  : BaseDns(face, keyChain, nextStageOnFailure)
{
}

void
GuessFromIdentityName::start()
{
  std::cerr << "Trying to find home router based on the default identity name..." << std::endl;

  Name identity = m_keyChain.getDefaultIdentity();

  std::ostringstream serverName;
  for (auto i = identity.rbegin(); i != identity.rend(); ++i) {
    serverName << i->toUri() << ".";
  }
  serverName << "_homehub._autoconf.named-data.net";

  try {
    std::string hubUri = BaseDns::querySrvRr(serverName.str());
    this->connectToHub(hubUri);
  }
  catch (const BaseDns::Error& e) {
    m_nextStageOnFailure(std::string("Failed to find a home router based on the default identity "
                                     "name (") + e.what() + ")");
  }
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn
