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

#ifndef NFD_TESTS_MANAGER_COMMON_FIXTURE_HPP
#define NFD_TESTS_MANAGER_COMMON_FIXTURE_HPP

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"
#include "core/manager-base.hpp"

#include <ndn-cxx/mgmt/dispatcher.hpp>
#include <ndn-cxx/security/command-interest-signer.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace tests {

/** \brief a fixture that provides a CommandInterestSigner
 */
class CommandInterestSignerFixture : public IdentityManagementTimeFixture
{
protected:
  CommandInterestSignerFixture();

  /** \brief sign a command Interest
   *  \param name command name include prefix and parameters
   *  \param identity signing identity
   *  \return a command Interest
   */
  Interest
  makeCommandInterest(const Name& name, const Name& identity = DEFAULT_COMMAND_SIGNER_IDENTITY);

  /** \brief create a ControlCommand request
   *  \param commandName command name including prefix, such as "/localhost/nfd/fib/add-nexthop"
   *  \param params command parameters
   *  \param identity signing identity
   *  \return a command Interest
   */
  Interest
  makeControlCommandRequest(Name commandName, const ControlParameters& params,
                            const Name& identity = DEFAULT_COMMAND_SIGNER_IDENTITY);

protected:
  static const Name DEFAULT_COMMAND_SIGNER_IDENTITY;

private:
  ndn::security::CommandInterestSigner m_commandInterestSigner;
};

/**
 * @brief a collection of common functions shared by all manager's testing fixtures.
 */
class ManagerCommonFixture : public CommandInterestSignerFixture
{
public: // initialize
  ManagerCommonFixture();

  /**
   * @brief set topPrefix to the dispatcher.
   *
   * after setting @param topPrefix, call advanceClocks to ensure all added filters take effects.
   *
   * @param topPrefix top prefix for the dispatcher
   */
  void
  setTopPrefix(const Name& topPrefix);

public: // test
  /**
   * @brief cause management to receive an Interest
   *
   * call DummyClientFace::receive to receive Interest and then call advanceClocks to ensure
   * the Interest dispatched
   *
   * @param interest the Interest to receive
   */
  void
  receiveInterest(const Interest& interest);

public: // verify
  ControlResponse
  makeResponse(uint32_t code, const std::string& text,
               const ControlParameters& parameters);

  enum class CheckResponseResult
  {
    OK,
    OUT_OF_BOUNDARY,
    WRONG_NAME,
    WRONG_CONTENT_TYPE,
    INVALID_RESPONSE,
    WRONG_CODE,
    WRONG_TEXT,
    WRONG_BODY_SIZE,
    WRONG_BODY_VALUE
  };

  /**
   * @brief check a specified response data with the expected ControlResponse
   *
   * @param idx the index of the specified Data in m_responses
   * @param expectedDataName the expected name of this Data
   * @param expectedResponse the expected ControlResponse
   * @param expectedContentType the expected content type of this Data, use -1 to skip this check
   *
   * @retval OK the response at the specified index can be decoded from the response data,
   *            and its code, text and response body are all matched with the expected response
   * @retval OUT_OF_BOUNDARY the specified index out of boundary
   * @retval WRONG_NAME the name of the specified response data does not match
   * @retval WRONG_CONTENT_TYPE the content type of the specified response data does not match
   * @retval INVALID_RESPONSE the data name matches but it fails in decoding a ControlResponse from
   *                          the content of the specified response data
   * @retval WRONG_CODE a valid ControlResponse can be decoded but has a wrong code
   * @retval WRONG_TEXT a valid ControlResponse can be decoded but has a wrong text
   * @retval WRONG_BODY_SIZE the body size of decoded ControlResponse does not match
   * @retval WRONT_BODY_VALUE the body value of decoded ControlResponse does not match
   */
  CheckResponseResult
  checkResponse(size_t idx,
                const Name& expectedName,
                const ControlResponse& expectedResponse,
                int expectedContentType = -1);

  /**
   * @brief concatenate specified response Data into a single block
   *
   * @param startIndex the start index in m_responses
   * @param nResponses the number of response to concatenate
   *
   * @return the generated block
   */
  Block
  concatenateResponses(size_t startIndex = 0, size_t nResponses = 0);

protected:
  ndn::util::DummyClientFace m_face;
  ndn::mgmt::Dispatcher m_dispatcher;
  std::vector<Data>& m_responses; ///< a reference of m_face->sentData
};

std::ostream&
operator<<(std::ostream& os, const ManagerCommonFixture::CheckResponseResult& result);

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_MANAGER_COMMON_FIXTURE_HPP
