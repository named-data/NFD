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

#ifndef NFD_TESTS_NFD_MGMT_VALIDATION_COMMON_HPP
#define NFD_TESTS_NFD_MGMT_VALIDATION_COMMON_HPP

#include "common.hpp"
#include <ndn-cxx/util/command-interest-generator.hpp>

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

#endif // NFD_TESTS_NFD_MGMT_VALIDATION_COMMON_HPP
