/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef VALIDATION_COMMON_HPP
#define VALIDATION_COMMON_HPP

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


template<typename T>
class CommandFixture : public T
{
public:
  virtual
  ~CommandFixture()
  {
    m_keys.deleteIdentity(m_identityName);

  }

  void
  generateCommand(Interest& interest)
  {
    m_generator.generateWithIdentity(interest, m_identityName);
  }

  const Name&
  getIdentityName() const
  {
    return m_identityName;
  }

protected:
  CommandFixture()
    : m_identityName("/unit-test/CommandFixture/id"),
      m_certificate(m_keys.getCertificate(m_keys.createIdentity(m_identityName)))
  {

  }

protected:
  ndn::KeyChain m_keys;
  const Name m_identityName;
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

} //namespace tests
} // namespace nfd

#endif // VALIDATION_COMMON_HPP
