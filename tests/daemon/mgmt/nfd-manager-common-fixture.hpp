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

#ifndef NFD_TESTS_NFD_MGMT_NFD_MANAGER_COMMON_HPP
#define NFD_TESTS_NFD_MGMT_NFD_MANAGER_COMMON_HPP

#include "manager-common-fixture.hpp"
#include "fw/forwarder.hpp"
#include "mgmt/command-validator.hpp"

namespace nfd {
namespace tests {

/**
 * @brief a base class shared by all NFD manager's testing fixtures.
 */
class NfdManagerCommonFixture : public ManagerCommonFixture
{
public:
  NfdManagerCommonFixture()
    : m_topPrefix("/localhost/nfd")
  {
  }

  /**
   * @brief configure an interest rule for the module.
   *
   * set top prefix before set privilege.
   *
   * @param privilege the module name
   */
  void
  setPrivilege(const std::string& privilege)
  {
    setTopPrefix(m_topPrefix);

    std::string regex("^");
    for (auto component : m_topPrefix) {
      regex += "<" + component.toUri() + ">";
    }

    m_validator.addInterestRule(regex + "<" + privilege + ">", *m_certificate);
  }

protected:
  Forwarder m_forwarder;
  CommandValidator m_validator;
  Name m_topPrefix;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_NFD_MGMT_NFD_MANAGER_COMMON_HPP
