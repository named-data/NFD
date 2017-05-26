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

#include "identity-management-fixture.hpp"
#include <ndn-cxx/security/pib/identity.hpp>
#include <ndn-cxx/security/pib/key.hpp>
#include <ndn-cxx/security/pib/pib.hpp>
#include <ndn-cxx/security/v2/certificate.hpp>
#include <ndn-cxx/util/io.hpp>
#include <boost/filesystem.hpp>

namespace nfd {
namespace tests {

IdentityManagementFixture::IdentityManagementFixture()
  : m_keyChain("pib-memory:", "tpm-memory:")
{
  m_keyChain.createIdentity("/DEFAULT");
}

IdentityManagementFixture::~IdentityManagementFixture()
{
  boost::system::error_code ec;
  for (const auto& certFile : m_certFiles) {
    boost::filesystem::remove(certFile, ec); // ignore error
  }
}

bool
IdentityManagementFixture::addIdentity(const Name& identity, const ndn::KeyParams& params)
{
  try {
    m_keyChain.createIdentity(identity, params);
    return true;
  }
  catch (const std::runtime_error&) {
    return false;
  }
}

bool
IdentityManagementFixture::saveIdentityCertificate(const Name& identity, const std::string& filename, bool wantAdd)
{
  ndn::security::v2::Certificate cert;
  try {
    cert = m_keyChain.getPib().getIdentity(identity).getDefaultKey().getDefaultCertificate();
  }
  catch (const ndn::security::Pib::Error&) {
    if (wantAdd && this->addIdentity(identity)) {
      return this->saveIdentityCertificate(identity, filename, false);
    }
    return false;
  }

  m_certFiles.push_back(filename);
  try {
    ndn::io::save(cert, filename);
    return true;
  }
  catch (const ndn::io::Error&) {
    return false;
  }
}

} // namespace tests
} // namespace nfd
