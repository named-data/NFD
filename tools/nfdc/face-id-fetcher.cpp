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

#include "face-id-fetcher.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <ndn-cxx/mgmt/nfd/face-query-filter.hpp>
#include <ndn-cxx/mgmt/nfd/face-status.hpp>
#include <ndn-cxx/util/segment-fetcher.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

FaceIdFetcher::FaceIdFetcher(ndn::Face& face,
                             ndn::nfd::Controller& controller,
                             bool allowCreate,
                             const SuccessCallback& onSucceed,
                             const FailureCallback& onFail)
  : m_face(face)
  , m_controller(controller)
  , m_allowCreate(allowCreate)
  , m_onSucceed(onSucceed)
  , m_onFail(onFail)
{
}

void
FaceIdFetcher::start(ndn::Face& face,
                     ndn::nfd::Controller& controller,
                     const std::string& input,
                     bool allowCreate,
                     const SuccessCallback& onSucceed,
                     const FailureCallback& onFail)
{
  // 1. Try parse input as FaceId, if input is FaceId, succeed with parsed FaceId
  // 2. Try parse input as FaceUri, if input is not FaceUri, fail
  // 3. Canonize faceUri
  // 4. If canonization fails, fail
  // 5. Query for face
  // 6. If query succeeds and finds a face, succeed with found FaceId
  // 7. Create face
  // 8. If face creation succeeds, succeed with created FaceId
  // 9. Fail

  boost::regex e("^[a-z0-9]+\\:.*");
  if (!boost::regex_match(input, e)) {
    try {
      uint32_t faceId = boost::lexical_cast<uint32_t>(input);
      onSucceed(faceId);
      return;
    }
    catch (const boost::bad_lexical_cast&) {
      onFail("No valid faceId or faceUri is provided");
      return;
    }
  }
  else {
    FaceUri faceUri;
    if (!faceUri.parse(input)) {
      onFail("FaceUri parse failed");
      return;
    }

    auto fetcher = new FaceIdFetcher(std::ref(face), std::ref(controller),
                                     allowCreate, onSucceed, onFail);
    fetcher->startGetFaceId(faceUri);
  }
}

void
FaceIdFetcher::startGetFaceId(const FaceUri& faceUri)
{
  faceUri.canonize(bind(&FaceIdFetcher::onCanonizeSuccess, this, _1),
                   bind(&FaceIdFetcher::onCanonizeFailure, this, _1),
                   m_face.getIoService(), time::seconds(4));
}

void
FaceIdFetcher::onCanonizeSuccess(const FaceUri& canonicalUri)
{
  ndn::Name queryName("/localhost/nfd/faces/query");
  ndn::nfd::FaceQueryFilter queryFilter;
  queryFilter.setRemoteUri(canonicalUri.toString());
  queryName.append(queryFilter.wireEncode());

  ndn::Interest interestPacket(queryName);
  interestPacket.setMustBeFresh(true);
  interestPacket.setInterestLifetime(time::milliseconds(4000));
  auto interest = std::make_shared<ndn::Interest>(interestPacket);

  ndn::util::SegmentFetcher::fetch(
    m_face, *interest, m_validator,
    bind(&FaceIdFetcher::onQuerySuccess, this, _1, canonicalUri),
    bind(&FaceIdFetcher::onQueryFailure, this, _1, canonicalUri));
}

void
FaceIdFetcher::onCanonizeFailure(const std::string& reason)
{
  fail("Canonize faceUri failed : " + reason);
}

void
FaceIdFetcher::onQuerySuccess(const ndn::ConstBufferPtr& data,
                              const FaceUri& canonicalUri)
{
  size_t offset = 0;
  bool isOk = false;
  ndn::Block block;
  std::tie(isOk, block) = ndn::Block::fromBuffer(data, offset);

  if (!isOk) {
    if (m_allowCreate) {
      startFaceCreate(canonicalUri);
    }
    else {
      fail("Fail to find faceId");
    }
  }
  else {
    try {
      ndn::nfd::FaceStatus status(block);
      succeed(status.getFaceId());
    }
    catch (const ndn::tlv::Error& e) {
      std::string errorMessage(e.what());
      fail("ERROR: " + errorMessage);
    }
  }
}

void
FaceIdFetcher::onQueryFailure(uint32_t errorCode,
                              const FaceUri& canonicalUri)
{
  std::stringstream ss;
  ss << "Cannot fetch data (code " << errorCode << ")";
  fail(ss.str());
}

void
FaceIdFetcher::onFaceCreateError(const ndn::nfd::ControlResponse& response,
                                 const std::string& message)
{
  std::stringstream ss;
  ss << message << " : " << response.getText() << " (code " << response.getCode() << ")";
  fail(ss.str());
}

void
FaceIdFetcher::startFaceCreate(const FaceUri& canonicalUri)
{
  ndn::nfd::ControlParameters parameters;
  parameters.setUri(canonicalUri.toString());

  m_controller.start<ndn::nfd::FaceCreateCommand>(parameters,
    [this] (const ndn::nfd::ControlParameters& result) { succeed(result.getFaceId()); },
    bind(&FaceIdFetcher::onFaceCreateError, this, _1, "Face creation failed"));
}

void
FaceIdFetcher::succeed(uint32_t faceId)
{
  m_onSucceed(faceId);
  delete this;
}

void
FaceIdFetcher::fail(const std::string& reason)
{
  m_onFail(reason);
  delete this;
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
