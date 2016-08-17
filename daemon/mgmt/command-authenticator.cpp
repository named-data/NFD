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

#include "command-authenticator.hpp"
#include "core/logger.hpp"

#include <ndn-cxx/security/identity-certificate.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/io.hpp>

#include <boost/filesystem.hpp>

namespace nfd {

NFD_LOG_INIT("CommandAuthenticator");
// INFO: configuration change, etc
// DEBUG: per authentication request result

shared_ptr<CommandAuthenticator>
CommandAuthenticator::create()
{
  return shared_ptr<CommandAuthenticator>(new CommandAuthenticator());
}

CommandAuthenticator::CommandAuthenticator()
  : m_validator(make_unique<ndn::ValidatorNull>())
{
}

void
CommandAuthenticator::setConfigFile(ConfigFile& configFile)
{
  configFile.addSectionHandler("authorizations",
    bind(&CommandAuthenticator::processConfig, this, _1, _2, _3));
}

void
CommandAuthenticator::processConfig(const ConfigSection& section, bool isDryRun, const std::string& filename)
{
  if (!isDryRun) {
    NFD_LOG_INFO("clear-authorizations");
    for (auto& kv : m_moduleAuth) {
      kv.second.allowAny = false;
      kv.second.certs.clear();
    }
  }

  if (section.empty()) {
    BOOST_THROW_EXCEPTION(ConfigFile::Error("'authorize' is missing under 'authorizations'"));
  }

  int authSectionIndex = 0;
  for (const auto& kv : section) {
    if (kv.first != "authorize") {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(
        "'" + kv.first + "' section is not permitted under 'authorizations'"));
    }
    const ConfigSection& authSection = kv.second;

    std::string certfile;
    try {
      certfile = authSection.get<std::string>("certfile");
    }
    catch (const boost::property_tree::ptree_error&) {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(
        "'certfile' is missing under authorize[" + to_string(authSectionIndex) + "]"));
    }

    bool isAny = false;
    shared_ptr<ndn::IdentityCertificate> cert;
    if (certfile == "any") {
      isAny = true;
      NFD_LOG_WARN("'certfile any' is intended for demo purposes only and "
                   "SHOULD NOT be used in production environments");
    }
    else {
      using namespace boost::filesystem;
      path certfilePath = absolute(certfile, path(filename).parent_path());
      cert = ndn::io::load<ndn::IdentityCertificate>(certfilePath.string());
      if (cert == nullptr) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error(
          "cannot load certfile " + certfilePath.string() +
          " for authorize[" + to_string(authSectionIndex) + "]"));
      }
    }

    const ConfigSection* privSection = nullptr;
    try {
      privSection = &authSection.get_child("privileges");
    }
    catch (const boost::property_tree::ptree_error&) {
      BOOST_THROW_EXCEPTION(ConfigFile::Error(
        "'privileges' is missing under authorize[" + to_string(authSectionIndex) + "]"));
    }

    if (privSection->empty()) {
      NFD_LOG_WARN("No privileges granted to certificate " << certfile);
    }
    for (const auto& kv : *privSection) {
      const std::string& module = kv.first;
      auto found = m_moduleAuth.find(module);
      if (found == m_moduleAuth.end()) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error(
          "unknown module '" + module + "' under authorize[" + to_string(authSectionIndex) + "]"));
      }

      if (isDryRun) {
        continue;
      }

      if (isAny) {
        found->second.allowAny = true;
        NFD_LOG_INFO("authorize module=" << module << " signer=any");
      }
      else {
        const Name& keyName = cert->getPublicKeyName();
        found->second.certs.emplace(keyName, cert->getPublicKeyInfo());
        NFD_LOG_INFO("authorize module=" << module << " signer=" << keyName <<
                     " certfile=" << certfile);
      }
    }

    ++authSectionIndex;
  }
}

ndn::mgmt::Authorization
CommandAuthenticator::makeAuthorization(const std::string& module, const std::string& verb)
{
  m_moduleAuth[module]; // declares module, so that privilege is recognized

  auto self = this->shared_from_this();
  return [=] (const Name& prefix, const Interest& interest,
              const ndn::mgmt::ControlParameters* params,
              const ndn::mgmt::AcceptContinuation& accept,
              const ndn::mgmt::RejectContinuation& reject) {
    const AuthorizedCerts& authorized = self->m_moduleAuth.at(module);
    if (authorized.allowAny) {
      NFD_LOG_DEBUG("accept " << interest.getName() << " allowAny");
      accept("*");
      return;
    }

    bool isOk = false;
    Name keyName;
    std::tie(isOk, keyName) = CommandAuthenticator::extractKeyName(interest);
    if (!isOk) {
      NFD_LOG_DEBUG("reject " << interest.getName() << " bad-KeyLocator");
      reject(ndn::mgmt::RejectReply::SILENT);
      return;
    }

    auto found = authorized.certs.find(keyName);
    if (found == authorized.certs.end()) {
      NFD_LOG_DEBUG("reject " << interest.getName() << " signer=" << keyName << " not-authorized");
      reject(ndn::mgmt::RejectReply::STATUS403);
      return;
    }

    bool hasGoodSig = ndn::Validator::verifySignature(interest, found->second);
    if (!hasGoodSig) {
      NFD_LOG_DEBUG("reject " << interest.getName() << " signer=" << keyName << " bad-sig");
      reject(ndn::mgmt::RejectReply::STATUS403);
      return;
    }

    self->m_validator.validate(interest,
      bind([=] {
        NFD_LOG_DEBUG("accept " << interest.getName() << " signer=" << keyName);
        accept(keyName.toUri());
      }),
      bind([=] {
        NFD_LOG_DEBUG("reject " << interest.getName() << " signer=" << keyName << " invalid-timestamp");
        reject(ndn::mgmt::RejectReply::STATUS403);
      }));
  };
}

std::pair<bool, Name>
CommandAuthenticator::extractKeyName(const Interest& interest)
{
  const Name& name = interest.getName();
  if (name.size() < ndn::signed_interest::MIN_LENGTH) {
    return {false, Name()};
  }

  ndn::SignatureInfo sig;
  try {
    sig.wireDecode(name[ndn::signed_interest::POS_SIG_INFO].blockFromValue());
  }
  catch (const tlv::Error&) {
    return {false, Name()};
  }

  if (!sig.hasKeyLocator()) {
    return {false, Name()};
  }

  const ndn::KeyLocator& keyLocator = sig.getKeyLocator();
  if (keyLocator.getType() != ndn::KeyLocator::KeyLocator_Name) {
    return {false, Name()};
  }

  try {
    return {true, ndn::IdentityCertificate::certificateNameToPublicKeyName(keyLocator.getName())};
  }
  catch (const ndn::IdentityCertificate::Error&) {
    return {false, Name()};
  }
}

} // namespace nfd
