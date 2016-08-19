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

#ifndef NFD_TESTS_TOOLS_NFD_STATUS_MODULE_FIXTURE_HPP
#define NFD_TESTS_TOOLS_NFD_STATUS_MODULE_FIXTURE_HPP

#include "nfdc/module.hpp"
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"

namespace nfd {
namespace tools {
namespace nfdc {
namespace tests {

using namespace nfd::tests;
using ndn::Face;
using ndn::KeyChain;
using ndn::Validator;
using ndn::ValidatorNull;
using ndn::util::DummyClientFace;
using boost::test_tools::output_test_stream;

class MakeValidatorNull
{
public:
  unique_ptr<ValidatorNull>
  operator()(Face&, KeyChain&) const
  {
    return make_unique<ValidatorNull>();
  };
};

/** \brief fixture to test a \p Module
 *  \tparam MODULE a subclass of \p Module
 *  \tparam MakeValidator a callable to make a Validator for use in \p controller;
 *                        MakeValidator()(Face&, KeyChain&) should return a unique_ptr
 *                        to Validator or its subclass
 */
template<typename MODULE, typename MakeValidator = MakeValidatorNull>
class ModuleFixture : public UnitTestTimeFixture
                    , public IdentityManagementFixture
{
protected:
  typedef typename std::result_of<MakeValidator(Face&, KeyChain&)>::type ValidatorUniquePtr;

  ModuleFixture()
    : face(g_io, m_keyChain)
    , validator(MakeValidator()(face, m_keyChain))
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
    module.fetchStatus(controller, [this] { ++nFetchStatusSuccess; },
                       [this] (uint32_t code, const std::string& reason) {
                         BOOST_FAIL("fetchStatus failure " << code << " " << reason);
                       },
                       CommandOptions());
    this->advanceClocks(time::milliseconds(1));
  }

  /** \brief send one WireEncodable in reply to StatusDataset request
   *  \param prefix dataset prefix without version and segment
   *  \param payload payload block
   *  \note payload must fit in one Data
   *  \pre fetchStatus has been invoked, sendDataset has not been invoked
   */
  template<typename T>
  void
  sendDataset(const Name& prefix, const T& payload)
  {
    BOOST_CONCEPT_ASSERT((ndn::WireEncodable<T>));

    this->sendDatasetReply(prefix, payload.wireEncode());
  }

  /** \brief send two WireEncodables in reply to StatusDataset request
   *  \param prefix dataset prefix without version and segment
   *  \param payload1 first vector item
   *  \param payload2 second vector item
   *  \note all payloads must fit in one Data
   *  \pre fetchStatus has been invoked, sendDataset has not been invoked
   */
  template<typename T1, typename T2>
  void
  sendDataset(const Name& prefix, const T1& payload1, const T2& payload2)
  {
    BOOST_CONCEPT_ASSERT((ndn::WireEncodable<T1>));
    BOOST_CONCEPT_ASSERT((ndn::WireEncodable<T2>));

    ndn::encoding::EncodingBuffer buffer;
    payload2.wireEncode(buffer);
    payload1.wireEncode(buffer);

    this->sendDatasetReply(prefix, buffer.buf(), buffer.size());
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

private:
  /** \brief send a payload in reply to StatusDataset request
   *  \param prefix dataset prefix without version and segment
   *  \param contentArgs passed to Data::setContent
   */
  template<typename ...ContentArgs>
  void
  sendDatasetReply(const Name& prefix, ContentArgs&&...contentArgs)
  {
    Name name = prefix;
    name.appendVersion().appendSegment(0);

    // These warnings assist in debugging a `nFetchStatusSuccess != 1` check failure.
    // They usually indicate a misspelled prefix or incorrect timing in the test case.
    if (face.sentInterests.size() < 1) {
      BOOST_WARN_MESSAGE(false, "no Interest expressed");
    }
    else {
      BOOST_WARN_MESSAGE(face.sentInterests.back().getName().isPrefixOf(name),
                         "last Interest " << face.sentInterests.back().getName() <<
                         " cannot be satisfied by this Data " << name);
    }

    auto data = make_shared<Data>(name);
    data->setFinalBlockId(name[-1]);
    data->setContent(std::forward<ContentArgs>(contentArgs)...);
    this->signDatasetReply(*data);
    face.receive(*data);
  }

  virtual void
  signDatasetReply(Data& data)
  {
    signData(data);
  }

protected:
  DummyClientFace face;
  ValidatorUniquePtr validator;
  Controller controller;

  MODULE module;

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

#endif // NFD_TESTS_TOOLS_NFD_STATUS_MODULE_FIXTURE_HPP
