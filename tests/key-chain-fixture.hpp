/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#ifndef NFD_TESTS_KEY_CHAIN_FIXTURE_HPP
#define NFD_TESTS_KEY_CHAIN_FIXTURE_HPP

#include "core/common.hpp"

#include <ndn-cxx/security/key-chain.hpp>

namespace nfd {
namespace tests {

/** \brief A fixture providing an in-memory KeyChain.
 */
class KeyChainFixture
{
public:
  /** \brief add identity
   *  \return whether successful
   */
  bool
  addIdentity(const Name& identity,
              const ndn::KeyParams& params = ndn::KeyChain::getDefaultKeyParams());

  /** \brief save identity certificate to a file
   *  \param identity identity name
   *  \param filename file name, must be writable
   *  \param allowAdd if true, add new identity when necessary
   *  \return whether successful
   */
  bool
  saveIdentityCertificate(const Name& identity, const std::string& filename, bool allowAdd = false);

  /** \brief retrieve identity certificate as base64 string
   *  \param identity identity name
   *  \param allowAdd if true, add new identity when necessary
   *  \throw std::runtime_error identity does not exist and \p allowAdd is false
   */
  std::string
  getIdentityCertificateBase64(const Name& identity, bool allowAdd = false);

protected:
  KeyChainFixture();

  /** \brief deletes saved certificate files
   */
  ~KeyChainFixture();

protected:
  ndn::KeyChain m_keyChain;

private:
  std::vector<std::string> m_certFiles;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_KEY_CHAIN_FIXTURE_HPP
