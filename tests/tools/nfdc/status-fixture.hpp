/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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

#ifndef NFD_TESTS_TOOLS_NFDC_STATUS_FIXTURE_HPP
#define NFD_TESTS_TOOLS_NFDC_STATUS_FIXTURE_HPP

#include "mock-nfd-mgmt-fixture.hpp"
#include "nfdc/module.hpp"
#include <ndn-cxx/security/validator-null.hpp>

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using ndn::Face;
using ndn::KeyChain;
using ndn::security::v2::Validator;
using ndn::security::v2::ValidatorNull;
using boost::test_tools::output_test_stream;

class MakeValidatorNull
{
public:
  unique_ptr<ValidatorNull>
  operator()(Face&, KeyChain&) const
  {
    return make_unique<ValidatorNull>();
  }
};

/** \brief fixture to test status fetching routines in a \p Module
 *  \tparam M a subclass of \p Module
 *  \tparam MakeValidator a callable to make a Validator for use in \p controller;
 *                        MakeValidator()(Face&, KeyChain&) should return a unique_ptr
 *                        to Validator or its subclass
 */
template<typename M, typename MakeValidator = MakeValidatorNull>
class StatusFixture : public MockNfdMgmtFixture
{
protected:
  using ValidatorUniquePtr = typename std::result_of<MakeValidator(Face&, KeyChain&)>::type;

  StatusFixture()
    : validator(MakeValidator()(face, m_keyChain))
    , controller(face, m_keyChain, *validator)
    , nFetchStatusSuccess(0)
  {
  }

protected: // status fetching
  /** \brief start fetching status
   *
   *  A test case should call \p fetchStatus, \p sendDataset, and \p prepareStatusOutput
   *  in this order, and then check \p statusXml and \p statusText contain the correct outputs.
   *  No advanceClocks is needed in between, as they are handled by the fixture.
   */
  void
  fetchStatus()
  {
    nFetchStatusSuccess = 0;
    module.fetchStatus(controller,
                       [this] { ++nFetchStatusSuccess; },
                       [] (uint32_t code, const std::string& reason) {
                         BOOST_FAIL("fetchStatus failure " << code << " " << reason);
                       },
                       CommandOptions());
    this->advanceClocks(time::milliseconds(1));
  }

  /** \brief prepare status output as XML and text
   *  \pre sendDataset has been invoked
   */
  void
  prepareStatusOutput()
  {
    this->advanceClocks(time::milliseconds(1));
    BOOST_REQUIRE_EQUAL(nFetchStatusSuccess, 1);

    statusXml.str("");
    module.formatStatusXml(statusXml);
    statusText.str("");
    module.formatStatusText(statusText);
  }

protected:
  ValidatorUniquePtr validator;
  Controller controller;

  M module;

  int nFetchStatusSuccess;
  output_test_stream statusXml;
  output_test_stream statusText;
};

/** \brief strips leading spaces on every line in expected XML
 *
 *  This allows expected XML to be written as:
 *  \code
 *  const std::string STATUS_XML = stripXmlSpaces(R"XML(
 *    <rootElement>
 *      <element>value</element>
 *    </rootElement>
 *  )XML");
 *  \endcode
 *  And \p STATUS_XML would be assigned:
 *  \code
 *  "<rootElement><element>value</element></rootElement>"
 *  \endcode
 */
inline std::string
stripXmlSpaces(const std::string& xml)
{
  std::string s;
  bool isSkipping = true;
  std::copy_if(xml.begin(), xml.end(), std::back_inserter(s),
               [&isSkipping] (char ch) {
                 if (ch == '\n') {
                   isSkipping = true;
                 }
                 else if (ch != ' ') {
                   isSkipping = false;
                 }
                 return !isSkipping;
               });
  return s;
}

} // namespace tests
} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TESTS_TOOLS_NFDC_STATUS_FIXTURE_HPP
