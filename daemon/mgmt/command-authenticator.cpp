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

#include "command-authenticator.hpp"
#include "core/logger.hpp"

#include <ndn-cxx/tag.hpp>
#include <ndn-cxx/security/v2/certificate-fetcher-offline.hpp>
#include <ndn-cxx/security/v2/certificate-request.hpp>
#include <ndn-cxx/security/v2/validation-policy.hpp>
#include <ndn-cxx/security/v2/validation-policy-accept-all.hpp>
#include <ndn-cxx/security/v2/validation-policy-command-interest.hpp>
#include <ndn-cxx/security/v2/validator.hpp>
#include <ndn-cxx/util/io.hpp>

#include <boost/filesystem.hpp>

namespace sec2 = ndn::security::v2;

namespace nfd {

NFD_LOG_INIT("CommandAuthenticator");
// INFO: configuration change, etc
// DEBUG: per authentication request result

/** \brief an Interest tag to indicate command signer
 */
using SignerTag = ndn::SimpleTag<Name, 20>;

/** \brief obtain signer from SignerTag attached to Interest, if available
 */
static optional<std::string>
getSignerFromTag(const Interest& interest)
{
  shared_ptr<SignerTag> signerTag = interest.getTag<SignerTag>();
  if (signerTag == nullptr) {
    return nullopt;
  }
  else {
    return signerTag->get().toUri();
  }
}

/** \brief a validation policy that only permits Interest signed by a trust anchor
 */
class CommandAuthenticatorValidationPolicy : public sec2::ValidationPolicy
{
public:
  void
  checkPolicy(const Interest& interest, const shared_ptr<sec2::ValidationState>& state,
              const ValidationContinuation& continueValidation) final
  {
    Name klName = getKeyLocatorName(interest, *state);
    if (!state->getOutcome()) { // already failed
      return;
    }

    // SignerTag must be placed on the 'original Interest' in ValidationState to be available for
    // InterestValidationSuccessCallback. The 'interest' parameter refers to a different instance
    // which is copied into 'original Interest'.
    auto state1 = dynamic_pointer_cast<sec2::InterestValidationState>(state);
    state1->getOriginalInterest().setTag(make_shared<SignerTag>(klName));

    continueValidation(make_shared<sec2::CertificateRequest>(Interest(klName)), state);
  }

  void
  checkPolicy(const Data& data, const shared_ptr<sec2::ValidationState>& state,
              const ValidationContinuation& continueValidation) final
  {
    // Non-certificate Data are not handled by CommandAuthenticator.
    // Non-anchor certificates cannot be retrieved by offline fetcher.
    BOOST_ASSERT_MSG(false, "Data should not be passed to this policy");
  }
};

shared_ptr<CommandAuthenticator>
CommandAuthenticator::create()
{
  return shared_ptr<CommandAuthenticator>(new CommandAuthenticator);
}

CommandAuthenticator::CommandAuthenticator() = default;

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
    for (auto& kv : m_validators) {
      kv.second = make_shared<sec2::Validator>(
        make_unique<sec2::ValidationPolicyCommandInterest>(make_unique<CommandAuthenticatorValidationPolicy>()),
        make_unique<sec2::CertificateFetcherOffline>());
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
    shared_ptr<sec2::Certificate> cert;
    if (certfile == "any") {
      isAny = true;
      NFD_LOG_WARN("'certfile any' is intended for demo purposes only and "
                   "SHOULD NOT be used in production environments");
    }
    else {
      using namespace boost::filesystem;
      path certfilePath = absolute(certfile, path(filename).parent_path());
      cert = ndn::io::load<sec2::Certificate>(certfilePath.string());
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
      auto found = m_validators.find(module);
      if (found == m_validators.end()) {
        BOOST_THROW_EXCEPTION(ConfigFile::Error(
          "unknown module '" + module + "' under authorize[" + to_string(authSectionIndex) + "]"));
      }

      if (isDryRun) {
        continue;
      }

      if (isAny) {
        found->second = make_shared<sec2::Validator>(make_unique<sec2::ValidationPolicyAcceptAll>(),
                                                     make_unique<sec2::CertificateFetcherOffline>());
        NFD_LOG_INFO("authorize module=" << module << " signer=any");
      }
      else {
        const Name& keyName = cert->getKeyName();
        sec2::Certificate certCopy = *cert;
        found->second->loadAnchor(certfile, std::move(certCopy));
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
  m_validators[module]; // declares module, so that privilege is recognized

  auto self = this->shared_from_this();
  return [=] (const Name&, const Interest& interest,
              const ndn::mgmt::ControlParameters*,
              const ndn::mgmt::AcceptContinuation& accept,
              const ndn::mgmt::RejectContinuation& reject) {
    auto validator = self->m_validators.at(module);
    auto successCb = [accept, validator] (const Interest& interest1) {
      auto signer1 = getSignerFromTag(interest1);
      BOOST_ASSERT(signer1 || // signer must be available unless 'certfile any'
                   dynamic_cast<sec2::ValidationPolicyAcceptAll*>(&validator->getPolicy()) != nullptr);
      std::string signer = signer1.value_or("*");
      NFD_LOG_DEBUG("accept " << interest1.getName() << " signer=" << signer);
      accept(signer);
    };
    auto failureCb = [reject] (const Interest& interest1, const sec2::ValidationError& err) {
      using ndn::mgmt::RejectReply;
      RejectReply reply = RejectReply::STATUS403;
      switch (err.getCode()) {
      case sec2::ValidationError::NO_SIGNATURE:
      case sec2::ValidationError::INVALID_KEY_LOCATOR:
        reply = RejectReply::SILENT;
        break;
      case sec2::ValidationError::POLICY_ERROR:
        if (interest1.getName().size() < ndn::command_interest::MIN_SIZE) { // "name too short"
          reply = RejectReply::SILENT;
        }
        break;
      }
      NFD_LOG_DEBUG("reject " << interest1.getName() << " signer=" <<
                    getSignerFromTag(interest1).value_or("?") << " reason=" << err);
      reject(reply);
    };

    if (validator) {
      validator->validate(interest, successCb, failureCb);
    }
    else {
      NFD_LOG_DEBUG("reject " << interest.getName() << " signer=" <<
                    getSignerFromTag(interest).value_or("?") << " reason=Unauthorized");
      reject(ndn::mgmt::RejectReply::STATUS403);
    }
  };
}

} // namespace nfd
