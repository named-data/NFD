/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_MGMT_NFD_MANAGER_COMMON_FIXTURE_HPP
#define NFD_TESTS_DAEMON_MGMT_NFD_MANAGER_COMMON_FIXTURE_HPP

#include "tests/manager-common-fixture.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/command-authenticator.hpp"

namespace nfd {
namespace tests {

/** \brief base fixture for testing an NFD Manager
 */
class NfdManagerCommonFixture : public ManagerCommonFixture
{
public:
  NfdManagerCommonFixture();

  /** \brief add /localhost/nfd as a top prefix to the dispatcher
   */
  void
  setTopPrefix();

  /** \brief grant m_identityName privilege to sign commands for the management module
   */
  void
  setPrivilege(const std::string& privilege);

protected:
  Forwarder m_forwarder;
  shared_ptr<CommandAuthenticator> m_authenticator;
};

class CommandSuccess
{
public:
  static ControlResponse
  getExpected()
  {
    return ControlResponse()
      .setCode(200)
      .setText("OK");
  }
};

template<int CODE>
class CommandFailure
{
public:
  static ControlResponse
  getExpected()
  {
    return ControlResponse()
      .setCode(CODE);
    // error description should not be checked
  }
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_MGMT_NFD_MANAGER_COMMON_FIXTURE_HPP
