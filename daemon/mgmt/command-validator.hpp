/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#ifndef NFD_DAEMON_MGMT_COMMAND_VALIDATOR_HPP
#define NFD_DAEMON_MGMT_COMMAND_VALIDATOR_HPP

#include "common.hpp"
#include "config-file.hpp"
#include <ndn-cxx/util/command-interest-validator.hpp>

namespace nfd {

class CommandValidator
{
public:

  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {

    }
  };

  CommandValidator();

  ~CommandValidator();

  void
  setConfigFile(ConfigFile& configFile);

  /**
   * \param section "authorizations" section to parse
   * \param isDryRun true if performing a dry run of configuration, false otherwise
   * \param filename filename of configuration file
   * \throws ConfigFile::Error on parse error
   */
  void
  onConfig(const ConfigSection& section, bool isDryRun, const std::string& filename);

  /**
   * \param privilege name of privilege to add
   * \throws CommandValidator::Error on duplicated privilege
   */
  void
  addSupportedPrivilege(const std::string& privilege);

  void
  addInterestRule(const std::string& regex,
                  const ndn::IdentityCertificate& certificate);

  void
  addInterestRule(const std::string& regex,
                  const Name& keyName,
                  const ndn::PublicKey& publicKey);

  void
  validate(const Interest& interest,
           const ndn::OnInterestValidated& onValidated,
           const ndn::OnInterestValidationFailed& onValidationFailed);

private:
  ndn::CommandInterestValidator m_validator;
  std::set<std::string> m_supportedPrivileges;
};

inline void
CommandValidator::addInterestRule(const std::string& regex,
                                  const ndn::IdentityCertificate& certificate)
{
  m_validator.addInterestRule(regex, certificate);
}

inline void
CommandValidator::addInterestRule(const std::string& regex,
                                  const Name& keyName,
                                  const ndn::PublicKey& publicKey)
{
  m_validator.addInterestRule(regex, keyName, publicKey);
}

inline void
CommandValidator::validate(const Interest& interest,
                           const ndn::OnInterestValidated& onValidated,
                           const ndn::OnInterestValidationFailed& onValidationFailed)
{
  m_validator.validate(interest, onValidated, onValidationFailed);
}

} // namespace nfd

#endif // NFD_DAEMON_MGMT_COMMAND_VALIDATOR_HPP
