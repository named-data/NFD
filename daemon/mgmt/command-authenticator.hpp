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

#ifndef NFD_DAEMON_MGMT_COMMAND_AUTHENTICATOR_HPP
#define NFD_DAEMON_MGMT_COMMAND_AUTHENTICATOR_HPP

#include "core/config-file.hpp"
#include <ndn-cxx/mgmt/dispatcher.hpp>

namespace ndn {
namespace security {
namespace v2 {
class Validator;
} // namespace v2
} // namespace security
} // namespace ndn

namespace nfd {

/** \brief provides ControlCommand authorization according to NFD configuration file
 */
class CommandAuthenticator : public enable_shared_from_this<CommandAuthenticator>, noncopyable
{
public:
  static shared_ptr<CommandAuthenticator>
  create();

  void
  setConfigFile(ConfigFile& configFile);

  /** \return an Authorization function for module/verb command
   *  \param module management module name
   *  \param verb command verb; currently it's ignored
   *  \note This must be called before parsing configuration file
   */
  ndn::mgmt::Authorization
  makeAuthorization(const std::string& module, const std::string& verb);

private:
  CommandAuthenticator();

  /** \brief process "authorizations" section
   *  \throw ConfigFile::Error on parse error
   */
  void
  processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

private:
  /// module => validator
  std::unordered_map<std::string, shared_ptr<ndn::security::v2::Validator>> m_validators;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_COMMAND_AUTHENTICATOR_HPP
