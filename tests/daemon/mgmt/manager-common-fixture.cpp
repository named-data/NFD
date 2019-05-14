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

#include "manager-common-fixture.hpp"

#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace tests {

const Name CommandInterestSignerFixture::DEFAULT_COMMAND_SIGNER_IDENTITY("/CommandInterestSignerFixture-identity");

CommandInterestSignerFixture::CommandInterestSignerFixture()
  : m_commandInterestSigner(m_keyChain)
{
  BOOST_REQUIRE(this->addIdentity(DEFAULT_COMMAND_SIGNER_IDENTITY));
}

Interest
CommandInterestSignerFixture::makeCommandInterest(const Name& name, const Name& identity)
{
  return m_commandInterestSigner.makeCommandInterest(name, ndn::security::signingByIdentity(identity));
}

Interest
CommandInterestSignerFixture::makeControlCommandRequest(Name commandName,
                                                        const ControlParameters& params,
                                                        const Name& identity)
{
  commandName.append(params.wireEncode());
  return this->makeCommandInterest(commandName, identity);
}

ManagerCommonFixture::ManagerCommonFixture()
  : m_face(g_io, m_keyChain, {true, true})
  , m_dispatcher(m_face, m_keyChain, ndn::security::SigningInfo())
  , m_responses(m_face.sentData)
{
}

void
ManagerCommonFixture::setTopPrefix()
{
  m_dispatcher.addTopPrefix("/localhost/nfd");
  advanceClocks(1_ms); // so that all filters are added
}

void
ManagerCommonFixture::receiveInterest(const Interest& interest)
{
  m_face.receive(interest);
  advanceClocks(1_ms);
}

ControlResponse
ManagerCommonFixture::makeResponse(uint32_t code, const std::string& text,
                                   const ControlParameters& parameters)
{
  return ControlResponse(code, text).setBody(parameters.wireEncode());
}

ManagerCommonFixture::CheckResponseResult
ManagerCommonFixture::checkResponse(size_t idx,
                                    const Name& expectedName,
                                    const ControlResponse& expectedResponse,
                                    int expectedContentType /*= -1*/)
{
  Data data;
  try {
    data = m_responses.at(idx);
  }
  catch (const std::out_of_range&) {
    BOOST_TEST_MESSAGE("response[" << idx << "] does not exist");
    return CheckResponseResult::OUT_OF_BOUNDARY;
  }

  if (data.getName() != expectedName) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong name " << data.getName());
    return CheckResponseResult::WRONG_NAME;
  }

  if (expectedContentType != -1 &&
      data.getContentType() != static_cast<uint32_t>(expectedContentType)) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong ContentType " << data.getContentType());
    return CheckResponseResult::WRONG_CONTENT_TYPE;
  }

  ControlResponse response;
  try {
    response.wireDecode(data.getContent().blockFromValue());
  }
  catch (const tlv::Error&) {
    BOOST_TEST_MESSAGE("response[" << idx << "] cannot be decoded");
    return CheckResponseResult::INVALID_RESPONSE;
  }

  if (response.getCode() != expectedResponse.getCode()) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong StatusCode " << response.getCode());
    return CheckResponseResult::WRONG_CODE;
  }

  if (response.getText() != expectedResponse.getText()) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong StatusText " << response.getText());
    return CheckResponseResult::WRONG_TEXT;
  }

  const Block& body = response.getBody();
  const Block& expectedBody = expectedResponse.getBody();
  if (body.value_size() != expectedBody.value_size()) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong body size " << body.value_size());
    return CheckResponseResult::WRONG_BODY_SIZE;
  }
  if (body.value_size() > 0 && memcmp(body.value(), expectedBody.value(), body.value_size()) != 0) {
    BOOST_TEST_MESSAGE("response[" << idx << "] has wrong body value");
    return CheckResponseResult::WRONG_BODY_VALUE;
  }

  return CheckResponseResult::OK;
}

Block
ManagerCommonFixture::concatenateResponses(size_t startIndex, size_t nResponses)
{
  while (m_responses.back().getName().at(-1) != m_responses.back().getFinalBlock()) {
    const Name& name = m_responses.back().getName();
    Name prefix = name.getPrefix(-1);
    uint64_t segmentNo = name.at(-1).toSegment() + 1;
    // request for the next segment
    receiveInterest(*makeInterest(prefix.appendSegment(segmentNo)));
  }

  size_t endIndex = startIndex + nResponses; // not included
  if (nResponses == startIndex || endIndex > m_responses.size()) {
    endIndex = m_responses.size();
  }

  ndn::EncodingBuffer encoder;
  size_t valueLength = 0;
  for (size_t i = startIndex; i < endIndex ; i ++) {
    valueLength += encoder.appendByteArray(m_responses[i].getContent().value(),
                                           m_responses[i].getContent().value_size());
  }
  encoder.prependVarNumber(valueLength);
  encoder.prependVarNumber(tlv::Content);
  return encoder.block();
}

std::ostream&
operator<<(std::ostream& os, ManagerCommonFixture::CheckResponseResult result)
{
  switch (result) {
    case ManagerCommonFixture::CheckResponseResult::OK:
      return os << "OK";
    case ManagerCommonFixture::CheckResponseResult::OUT_OF_BOUNDARY:
      return os << "OUT_OF_BOUNDARY";
    case ManagerCommonFixture::CheckResponseResult::WRONG_NAME:
      return os << "WRONG_NAME";
    case ManagerCommonFixture::CheckResponseResult::WRONG_CONTENT_TYPE:
      return os << "WRONG_CONTENT_TYPE";
    case ManagerCommonFixture::CheckResponseResult::INVALID_RESPONSE:
      return os << "INVALID_RESPONSE";
    case ManagerCommonFixture::CheckResponseResult::WRONG_CODE:
      return os << "WRONG_CODE";
    case ManagerCommonFixture::CheckResponseResult::WRONG_TEXT:
      return os << "WRONG_TEXT";
    case ManagerCommonFixture::CheckResponseResult::WRONG_BODY_SIZE:
      return os << "WRONG_BODY_SIZE";
    case ManagerCommonFixture::CheckResponseResult::WRONG_BODY_VALUE:
      return os << "WRONG_BODY_VALUE";
  }
  return os << static_cast<int>(result);
}

void
ManagerFixtureWithAuthenticator::setPrivilege(const std::string& privilege)
{
  saveIdentityCertificate(DEFAULT_COMMAND_SIGNER_IDENTITY, "ManagerCommonFixture.ndncert");

  const std::string& config = R"CONFIG(
    authorizations
    {
      authorize
      {
        certfile "ManagerCommonFixture.ndncert"
        privileges
        {
          )CONFIG" + privilege + R"CONFIG(
        }
      }
    }
  )CONFIG";

  ConfigFile cf;
  m_authenticator->setConfigFile(cf);
  cf.parse(config, false, "ManagerCommonFixture.authenticator.conf");
}

} // namespace tests
} // namespace nfd
