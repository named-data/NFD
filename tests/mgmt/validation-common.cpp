/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "validation-common.hpp"

#include <boost/test/unit_test.hpp>

namespace nfd {
namespace tests {

const Name CommandIdentityGlobalFixture::s_identityName("/unit-test/CommandFixture/id");
shared_ptr<ndn::IdentityCertificate> CommandIdentityGlobalFixture::s_certificate;

CommandIdentityGlobalFixture::CommandIdentityGlobalFixture()
{
  BOOST_ASSERT(!static_cast<bool>(s_certificate));
  s_certificate = m_keys.getCertificate(m_keys.createIdentity(s_identityName));
}

CommandIdentityGlobalFixture::~CommandIdentityGlobalFixture()
{
  s_certificate.reset();
  m_keys.deleteIdentity(s_identityName);
}

BOOST_GLOBAL_FIXTURE(CommandIdentityGlobalFixture);

} // namespace tests
} // namespace nfd

