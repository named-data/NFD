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

#ifndef NFD_TOOLS_NDN_AUTOCONFIG_SERVER_PROGRAM_HPP
#define NFD_TOOLS_NDN_AUTOCONFIG_SERVER_PROGRAM_HPP

#include "core/common.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn {
namespace tools {
namespace autoconfig_server {

struct Options
{
  FaceUri hubFaceUri;
  std::vector<Name> routablePrefixes;
};

class Program : noncopyable
{
public:
  Program(const Options& options, Face& face, KeyChain& keyChain);

  void
  run()
  {
    m_face.processEvents();
  }

private:
  void
  enableHubData(const FaceUri& hubFaceUri);

  void
  enableRoutablePrefixesDataset(const std::vector<Name>& routablePrefixes);

  void
  handlePrefixRegistrationFailure(const Name& prefix, const std::string& reason);

private:
  Face& m_face;
  KeyChain& m_keyChain;
  mgmt::Dispatcher m_dispatcher;
};

} // namespace autoconfig_server
} // namespace tools
} // namespace ndn

#endif // NFD_TOOLS_NDN_AUTOCONFIG_SERVER_PROGRAM_HPP
