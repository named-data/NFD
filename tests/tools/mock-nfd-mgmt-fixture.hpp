/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#ifndef NFD_TESTS_TOOLS_MOCK_NFD_MGMT_FIXTURE_HPP
#define NFD_TESTS_TOOLS_MOCK_NFD_MGMT_FIXTURE_HPP

#include "tests/test-common.hpp"
#include "tests/identity-management-fixture.hpp"

#include <ndn-cxx/util/dummy-client-face.hpp>

namespace nfd {
namespace tools {
namespace tests {

using namespace nfd::tests;
using ndn::nfd::ControlParameters;

/** \brief fixture to emulate NFD management
 */
class MockNfdMgmtFixture : public IdentityManagementTimeFixture
{
protected:
  MockNfdMgmtFixture()
    : face(g_io, m_keyChain,
           {true, false, bind(&MockNfdMgmtFixture::processEventsOverride, this, _1)})
  {
    face.onSendInterest.connect([=] (const Interest& interest) {
      g_io.post([=] {
        if (processInterest != nullptr) {
          processInterest(interest);
        }
      });
    });
  }

  virtual
  ~MockNfdMgmtFixture() = default;

protected: // ControlCommand
  /** \brief check the Interest is a command with specified prefix
   *  \retval nullopt last Interest is not the expected command
   *  \return command parameters
   */
  static optional<ControlParameters>
  parseCommand(const Interest& interest, const Name& expectedPrefix)
  {
    if (!expectedPrefix.isPrefixOf(interest.getName())) {
      return nullopt;
    }
    return ControlParameters(interest.getName().at(expectedPrefix.size()).blockFromValue());
  }

  /** \brief send successful response to a command Interest
   */
  void
  succeedCommand(const Interest& interest, const ControlParameters& parameters)
  {
    this->sendCommandReply(interest, 200, "OK", parameters.wireEncode());
  }

  /** \brief send failure response to a command Interest
   */
  void
  failCommand(const Interest& interest, uint32_t code, const std::string& text)
  {
    this->sendCommandReply(interest, {code, text});
  }

  /** \brief send failure response to a command Interest
   */
  void
  failCommand(const Interest& interest, uint32_t code, const std::string& text, const ControlParameters& body)
  {
    this->sendCommandReply(interest, code, text, body.wireEncode());
  }

protected: // StatusDataset
  /** \brief send an empty dataset in reply to StatusDataset request
   *  \param prefix dataset prefix without version and segment
   *  \pre Interest for dataset has been expressed, sendDataset has not been invoked
   */
  void
  sendEmptyDataset(const Name& prefix)
  {
    this->sendDatasetReply(prefix, nullptr, 0);
  }

  /** \brief send one WireEncodable in reply to StatusDataset request
   *  \param prefix dataset prefix without version and segment
   *  \param payload payload block
   *  \note payload must fit in one Data
   *  \pre Interest for dataset has been expressed, sendDataset has not been invoked
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
   *  \pre Interest for dataset has been expressed, sendDataset has not been invoked
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

private:
  virtual void
  processEventsOverride(time::milliseconds timeout)
  {
    if (timeout <= time::milliseconds::zero()) {
      // give enough time to finish execution
      timeout = time::seconds(30);
    }
    this->advanceClocks(time::milliseconds(100), timeout);
  }

  void
  sendCommandReply(const Interest& interest, const ndn::nfd::ControlResponse& resp)
  {
    auto data = makeData(interest.getName());
    data->setContent(resp.wireEncode());
    face.receive(*data);
  }

  void
  sendCommandReply(const Interest& interest, uint32_t code, const std::string& text,
                   const Block& body)
  {
    this->sendCommandReply(interest,
                           ndn::nfd::ControlResponse(code, text).setBody(body));
  }

  /** \brief send a payload in reply to StatusDataset request
   *  \param name dataset prefix without version and segment
   *  \param contentArgs passed to Data::setContent
   */
  template<typename ...ContentArgs>
  void
  sendDatasetReply(Name name, ContentArgs&&... contentArgs)
  {
    name.appendVersion().appendSegment(0);

    // These warnings assist in debugging when nfdc does not receive StatusDataset.
    // They usually indicate a misspelled prefix or incorrect timing in the test case.
    if (face.sentInterests.empty()) {
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
  ndn::util::DummyClientFace face;
  std::function<void(const Interest&)> processInterest;
};

} // namespace tests
} // namespace tools
} // namespace nfd

/** \brief require the command in \p interest has expected prefix
 *  \note This must be used in processInterest lambda, and the Interest must be named 'interest'.
 *  \return ControlParameters, or nullopt if \p interest does match \p expectedPrefix
 */
#define MOCK_NFD_MGMT_REQUIRE_COMMAND_IS(expectedPrefix) \
  [interest] { \
    auto params = parseCommand(interest, (expectedPrefix)); \
    BOOST_REQUIRE_MESSAGE(params, "Interest " << interest.getName() << \
                          " does not match command prefix " << (expectedPrefix)); \
    return *params; \
  } ()

#endif // NFD_TESTS_TOOLS_MOCK_NFD_MGMT_FIXTURE_HPP
