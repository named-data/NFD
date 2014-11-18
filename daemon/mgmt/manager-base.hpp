/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_DAEMON_MGMT_MANAGER_BASE_HPP
#define NFD_DAEMON_MGMT_MANAGER_BASE_HPP

#include "common.hpp"

#include "mgmt/command-validator.hpp"
#include "mgmt/internal-face.hpp"

#include <ndn-cxx/management/nfd-control-command.hpp>
#include <ndn-cxx/management/nfd-control-response.hpp>
#include <ndn-cxx/management/nfd-control-parameters.hpp>

namespace nfd {

using ndn::nfd::ControlCommand;
using ndn::nfd::ControlResponse;
using ndn::nfd::ControlParameters;

class InternalFace;

class ManagerBase
{
public:

  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  ManagerBase(shared_ptr<InternalFace> face,
              const std::string& privilege,
              ndn::KeyChain& keyChain);

  virtual
  ~ManagerBase();

  void
  onCommandValidationFailed(const shared_ptr<const Interest>& command,
                            const std::string& error);

protected:

  static bool
  extractParameters(const Name::Component& parameterComponent,
                    ControlParameters& extractedParameters);

  void
  setResponse(ControlResponse& response,
              uint32_t code,
              const std::string& text);
  void
  setResponse(ControlResponse& response,
              uint32_t code,
              const std::string& text,
              const Block& body);

  void
  sendResponse(const Name& name,
               const ControlResponse& response);

  void
  sendResponse(const Name& name,
               uint32_t code,
               const std::string& text);

  void
  sendResponse(const Name& name,
               uint32_t code,
               const std::string& text,
               const Block& body);

  void
  sendNack(const Name& name);

  virtual bool
  validateParameters(const ControlCommand& command,
                     ControlParameters& parameters);

PUBLIC_WITH_TESTS_ELSE_PROTECTED:
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

protected:
  shared_ptr<InternalFace> m_face;
  ndn::KeyChain& m_keyChain;
};

inline void
ManagerBase::setResponse(ControlResponse& response,
                         uint32_t code,
                         const std::string& text)
{
  response.setCode(code);
  response.setText(text);
}

inline void
ManagerBase::setResponse(ControlResponse& response,
                         uint32_t code,
                         const std::string& text,
                         const Block& body)
{
  setResponse(response, code, text);
  response.setBody(body);
}

inline void
ManagerBase::addInterestRule(const std::string& regex,
                             const ndn::IdentityCertificate& certificate)
{
  m_face->getValidator().addInterestRule(regex, certificate);
}

inline void
ManagerBase::addInterestRule(const std::string& regex,
                             const Name& keyName,
                             const ndn::PublicKey& publicKey)
{
  m_face->getValidator().addInterestRule(regex, keyName, publicKey);
}

inline void
ManagerBase::validate(const Interest& interest,
                      const ndn::OnInterestValidated& onValidated,
                      const ndn::OnInterestValidationFailed& onValidationFailed)
{
  m_face->getValidator().validate(interest, onValidated, onValidationFailed);
}


} // namespace nfd

#endif // NFD_DAEMON_MGMT_MANAGER_BASE_HPP
