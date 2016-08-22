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

#include "manager-common-fixture.hpp"
#include <ndn-cxx/security/signing-helpers.hpp>

namespace nfd {
namespace tests {

ManagerCommonFixture::ManagerCommonFixture()
  : m_face(getGlobalIoService(), m_keyChain, {true, true})
  , m_dispatcher(m_face, m_keyChain, ndn::security::SigningInfo())
  , m_responses(m_face.sentData)
  , m_identityName("/unit-test/ManagerCommonFixture/identity")
{
  BOOST_REQUIRE(this->addIdentity(m_identityName));
}

void
ManagerCommonFixture::setTopPrefix(const Name& topPrefix)
{
  m_dispatcher.addTopPrefix(topPrefix); // such that all filters are added
  advanceClocks(time::milliseconds(1));
}

shared_ptr<Interest>
ManagerCommonFixture::makeControlCommandRequest(Name commandName,
                                                const ControlParameters& parameters,
                                                const InterestHandler& beforeSigning)
{
  shared_ptr<Interest> command = makeInterest(commandName.append(parameters.wireEncode()));

  if (beforeSigning != nullptr) {
    beforeSigning(command);
  }

  m_keyChain.sign(*command, ndn::security::signingByIdentity(m_identityName));
  return command;
}

void
ManagerCommonFixture::receiveInterest(shared_ptr<Interest> interest)
{
  m_face.receive(*interest);
  advanceClocks(time::milliseconds(1));
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
    return CheckResponseResult::OUT_OF_BOUNDARY;
  }

  if (data.getName() != expectedName) {
    return CheckResponseResult::WRONG_NAME;
  }

  if (expectedContentType != -1 &&
      data.getContentType() != static_cast<uint32_t>(expectedContentType)) {
    return CheckResponseResult::WRONG_CONTENT_TYPE;
  }

  ControlResponse response;
  try {
    response.wireDecode(data.getContent().blockFromValue());
  }
  catch (const tlv::Error&) {
    return CheckResponseResult::INVALID_RESPONSE;
  }

  if (response.getCode() != expectedResponse.getCode()) {
    return CheckResponseResult::WRONG_CODE;
  }

  if (response.getText() != expectedResponse.getText()) {
    return CheckResponseResult::WRONG_TEXT;
  }

  const Block& body = response.getBody();
  const Block& expectedBody = expectedResponse.getBody();
  if (body.value_size() != expectedBody.value_size()) {
    return CheckResponseResult::WRONG_BODY_SIZE;
  }
  if (body.value_size() > 0 && memcmp(body.value(), expectedBody.value(), body.value_size()) != 0) {
    return CheckResponseResult::WRONG_BODY_VALUE;
  }

  return CheckResponseResult::OK;
}

Block
ManagerCommonFixture::concatenateResponses(size_t startIndex, size_t nResponses)
{
  auto isFinalSegment = [] (const Data& data) -> bool {
    const name::Component& lastComponent = data.getName().at(-1);
    return !lastComponent.isSegment() || lastComponent == data.getFinalBlockId();
  };

  while (!isFinalSegment(m_responses.back())) {
    const Name& name = m_responses.back().getName();
    Name prefix = name.getPrefix(-1);
    uint64_t segmentNo = name.at(-1).toSegment() + 1;
    // request for the next segment
    receiveInterest(makeInterest(prefix.appendSegment(segmentNo)));
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
operator<<(std::ostream &os, const ManagerCommonFixture::CheckResponseResult& result)
{
  switch (result) {
  case ManagerCommonFixture::CheckResponseResult::OK:
    os << "OK";
    break;
  case ManagerCommonFixture::CheckResponseResult::OUT_OF_BOUNDARY:
    os << "OUT_OF_BOUNDARY";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_NAME:
    os << "WRONG_NAME";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_CONTENT_TYPE:
    os << "WRONG_CONTENT_TYPE";
    break;
  case ManagerCommonFixture::CheckResponseResult::INVALID_RESPONSE:
    os << "INVALID_RESPONSE";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_CODE:
    os << "WRONG_CODE";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_TEXT:
    os << "WRONG_TEXT";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_BODY_SIZE:
    os << "WRONG_BODY_SIZE";
    break;
  case ManagerCommonFixture::CheckResponseResult::WRONG_BODY_VALUE:
    os << "WRONG_BODY_VALUE";
    break;
  default:
    break;
  };

  return os;
}

} // namespace tests
} // namespace nfd
