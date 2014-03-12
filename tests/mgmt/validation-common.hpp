/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_MGMT_VALIDATION_COMMON_HPP
#define NFD_TEST_MGMT_VALIDATION_COMMON_HPP

#include "common.hpp"
#include <ndn-cpp-dev/util/command-interest-generator.hpp>

namespace nfd {
namespace tests {

// class ValidatedManagementFixture
// {
// public:
//   ValidatedManagementFixture()
//     : m_validator(make_shared<ndn::CommandInterestValidator>())
//   {
//   }

//   virtual
//   ~ValidatedManagementFixture()
//   {
//   }

// protected:
//   shared_ptr<ndn::CommandInterestValidator> m_validator;
// };

/// a global fixture that holds the identity for CommandFixture
class CommandIdentityGlobalFixture
{
public:
  CommandIdentityGlobalFixture();

  ~CommandIdentityGlobalFixture();
  
  static const Name& getIdentityName()
  {
    return s_identityName;
  }
  
  static shared_ptr<ndn::IdentityCertificate> getCertificate()
  {
    BOOST_ASSERT(static_cast<bool>(s_certificate));
    return s_certificate;
  }

private:
  ndn::KeyChain m_keys;
  static const Name s_identityName;
  static shared_ptr<ndn::IdentityCertificate> s_certificate;
};

template<typename T>
class CommandFixture : public T
{
public:
  virtual
  ~CommandFixture()
  {
  }

  void
  generateCommand(Interest& interest)
  {
    m_generator.generateWithIdentity(interest, getIdentityName());
  }

  const Name&
  getIdentityName() const
  {
    return CommandIdentityGlobalFixture::getIdentityName();
  }

protected:
  CommandFixture()
    : m_certificate(CommandIdentityGlobalFixture::getCertificate())
  {
  }

protected:
  shared_ptr<ndn::IdentityCertificate> m_certificate;
  ndn::CommandInterestGenerator m_generator;
};

template <typename T>
class UnauthorizedCommandFixture : public CommandFixture<T>
{
public:
  UnauthorizedCommandFixture()
  {
  }

  virtual
  ~UnauthorizedCommandFixture()
  {
  }
};

} // namespace tests
} // namespace nfd

#endif // NFD_TEST_MGMT_VALIDATION_COMMON_HPP
