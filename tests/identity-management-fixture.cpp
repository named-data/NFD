/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2014 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "identity-management-fixture.hpp"

namespace nfd {
namespace tests {

IdentityManagementFixture::IdentityManagementFixture()
  : m_keyChain("sqlite3", "file")
{
}

IdentityManagementFixture::~IdentityManagementFixture()
{
  for (auto&& id : m_identities) {
    m_keyChain.deleteIdentity(id);
  }
}

bool
IdentityManagementFixture::addIdentity(const ndn::Name& identity, const ndn::KeyParams& params)
{
  try {
    m_keyChain.createIdentity(identity, params);
    m_identities.push_back(identity);
    return true;
  }
  catch (std::runtime_error&) {
    return false;
  }
}

} // namespace tests
} // namespace nfd
