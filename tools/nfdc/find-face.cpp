/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
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

#include "find-face.hpp"
#include "canonizer.hpp"
#include "format-helpers.hpp"

#include <ndn-cxx/util/logger.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

NDN_LOG_INIT(nfdc.FindFace);

FindFace::FindFace(ExecuteContext& ctx)
  : m_ctx(ctx)
{
}

FindFace::Code
FindFace::execute(const FaceUri& faceUri, bool allowMulti)
{
  FaceQueryFilter filter;
  filter.setRemoteUri(faceUri.toString());
  return this->execute(filter, allowMulti);
}

FindFace::Code
FindFace::execute(uint64_t faceId)
{
  FaceQueryFilter filter;
  filter.setFaceId(faceId);
  return this->execute(filter);
}

FindFace::Code
FindFace::execute(const ndn::any& faceIdOrUri, bool allowMulti)
{
  const uint64_t* faceId = ndn::any_cast<uint64_t>(&faceIdOrUri);
  if (faceId != nullptr) {
    return this->execute(*faceId);
  }
  else {
    return this->execute(ndn::any_cast<FaceUri>(faceIdOrUri), allowMulti);
  }
}

FindFace::Code
FindFace::execute(const FaceQueryFilter& filter, bool allowMulti)
{
  BOOST_ASSERT(m_res == Code::NOT_STARTED);
  m_res = Code::IN_PROGRESS;
  m_filter = filter;

  if (m_filter.hasRemoteUri()) {
    auto remoteUri = canonize("remote FaceUri", FaceUri(m_filter.getRemoteUri()));
    if (!remoteUri) {
      return m_res;
    }
    m_filter.setRemoteUri(remoteUri->toString());
  }

  if (m_filter.hasLocalUri()) {
    auto localUri = canonize("local FaceUri", FaceUri(m_filter.getLocalUri()));
    if (!localUri) {
      return m_res;
    }
    m_filter.setLocalUri(localUri->toString());
  }

  this->query();
  if (m_res == Code::OK) {
    if (m_results.size() == 0) {
      m_res = Code::NOT_FOUND;
      m_errorReason = "Face not found";
    }
    else if (m_results.size() > 1 && !allowMulti) {
      m_res = Code::AMBIGUOUS;
      m_errorReason = "Multiple faces match the query";
    }
  }
  return m_res;
}

optional<FaceUri>
FindFace::canonize(const std::string& fieldName, const FaceUri& uri)
{
  // We use a wrapper because we want to accept FaceUris that cannot be canonized
  if (!FaceUri::canCanonize(uri.getScheme())) {
    NDN_LOG_DEBUG("Using " << fieldName << "=" << uri << " without canonization");
    return uri;
  }

  optional<FaceUri> result;
  std::string error;
  std::tie(result, error) = nfdc::canonize(m_ctx, uri);

  if (result) {
    // Canonization succeeded
    return result;
  }
  else {
    // Canonization failed
    std::tie(m_res, m_errorReason) = canonizeErrorHelper(uri, error);
    return nullopt;
  }
}

void
FindFace::query()
{
  auto datasetCb = [this] (const std::vector<ndn::nfd::FaceStatus>& result) {
    m_res = Code::OK;
    m_results = result;
  };
  auto failureCb = [this] (uint32_t code, const std::string& reason) {
    m_res = Code::ERROR;
    m_errorReason = "Error " + to_string(code) + " when querying face: " + reason;
  };

  if (m_filter.empty()) {
    m_ctx.controller.fetch<ndn::nfd::FaceDataset>(
      datasetCb, failureCb, m_ctx.makeCommandOptions());
  }
  else {
    m_ctx.controller.fetch<ndn::nfd::FaceQueryDataset>(
      m_filter, datasetCb, failureCb, m_ctx.makeCommandOptions());
  }
  m_ctx.face.processEvents();
}

std::set<uint64_t>
FindFace::getFaceIds() const
{
  std::set<uint64_t> faceIds;
  for (const FaceStatus& faceStatus : m_results) {
    faceIds.insert(faceStatus.getFaceId());
  }
  return faceIds;
}

const FaceStatus&
FindFace::getFaceStatus() const
{
  BOOST_ASSERT(m_results.size() == 1);
  return m_results.front();
}

void
FindFace::printDisambiguation(std::ostream& os, DisambiguationStyle style) const
{
  text::Separator sep(" ", ", ");
  for (const auto& item : m_results) {
    os << sep;
    switch (style) {
      case DisambiguationStyle::LOCAL_URI:
        os << item.getFaceId() << " (local=" << item.getLocalUri() << ')';
        break;
    }
  }
}

} // namespace nfdc
} // namespace tools
} // namespace nfd
