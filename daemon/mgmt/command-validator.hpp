/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_COMMAND_VALIDATOR_HPP
#define NFD_MGMT_COMMAND_VALIDATOR_HPP

#include "config-file.hpp"
#include <ndn-cpp-dev/util/command-interest-validator.hpp>

namespace nfd {

class CommandValidator
{
public:

  class Error : public std::runtime_error
  {
  public:
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
   * \throws ConfigFile::Error on parse error
   */
  void
  onConfig(const ConfigSection& section, bool isDryRun);

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

#endif // NFD_MGMT_COMMAND_VALIDATOR_HPP
