/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_MGMT_MANAGER_BASE_HPP
#define NFD_MGMT_MANAGER_BASE_HPP

#include "common.hpp"

#include "mgmt/command-validator.hpp"
#include "mgmt/internal-face.hpp"

#include <ndn-cpp-dev/management/nfd-control-command.hpp>
#include <ndn-cpp-dev/management/nfd-control-response.hpp>
#include <ndn-cpp-dev/management/nfd-control-parameters.hpp>

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

  ManagerBase(shared_ptr<InternalFace> face, const std::string& privilege);

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

#endif // NFD_MGMT_MANAGER_BASE_HPP

