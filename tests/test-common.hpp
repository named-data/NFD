/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TEST_COMMON_HPP
#define NFD_TEST_COMMON_HPP

#include <boost/test/unit_test.hpp>
#include "core/global-io.hpp"
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
  fakeSignature.setValue(ndn::dataBlock(tlv::SignatureValue, reinterpret_cast<const uint8_t*>(0), 0));
  data->setSignature(fakeSignature);

  return data;
}

} // namespace tests
} // namespace nfd

#endif // NFD_TEST_COMMON_HPP
