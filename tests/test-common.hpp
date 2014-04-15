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

#ifndef NFD_TESTS_TEST_COMMON_HPP
#define NFD_TESTS_TEST_COMMON_HPP

#include <boost/test/unit_test.hpp>
#include "core/global-io.hpp"
#include "core/logger.hpp"
#include <ndn-cpp-dev/security/key-chain.hpp>

namespace nfd {
namespace tests {

/** \brief base test fixture
 *
 *  Every test case should be based on this fixture,
 *  to have per test case io_service initialization.
 */
class BaseFixture
{
protected:
  BaseFixture()
    : g_io(getGlobalIoService())
  {
  }

  ~BaseFixture()
  {
    resetGlobalIoService();
  }

protected:
  /// reference to global io_service
  boost::asio::io_service& g_io;
};


inline shared_ptr<Interest>
makeInterest(const Name& name)
{
  return make_shared<Interest>(name);
}

inline shared_ptr<Data>
makeData(const Name& name)
{
  shared_ptr<Data> data = make_shared<Data>(name);

  ndn::SignatureSha256WithRsa fakeSignature;
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue,
                                        reinterpret_cast<const uint8_t*>(0), 0));
  data->setSignature(fakeSignature);

  return data;
}

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_TEST_COMMON_HPP
