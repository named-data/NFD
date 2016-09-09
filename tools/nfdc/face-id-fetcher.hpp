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

#ifndef NFD_TOOLS_NFDC_FACE_ID_FETCHER_HPP
#define NFD_TOOLS_NFDC_FACE_ID_FETCHER_HPP

#include "core/common.hpp"
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/util/face-uri.hpp>
#include <ndn-cxx/security/validator-null.hpp>

namespace nfd {
namespace tools {
namespace nfdc {

using ndn::util::FaceUri;

class FaceIdFetcher
{
public:
  typedef std::function<void(uint32_t)> SuccessCallback;
  typedef std::function<void(const std::string&)> FailureCallback;

  /** \brief obtain FaceId from input
   *  \param face Reference to the Face that should be used to fetch data
   *  \param controller Reference to the controller that should be used to sign the Interest
   *  \param input User input, either FaceId or FaceUri
   *  \param allowCreate Whether creating face is allowed
   *  \param onSucceed Callback to be fired when faceId is obtained
   *  \param onFail Callback to be fired when an error occurs
   */
  static void
  start(ndn::Face& face,
        ndn::nfd::Controller& controller,
        const std::string& input,
        bool allowCreate,
        const SuccessCallback& onSucceed,
        const FailureCallback& onFail);

private:
  FaceIdFetcher(ndn::Face& face,
                ndn::nfd::Controller& controller,
                bool allowCreate,
                const SuccessCallback& onSucceed,
                const FailureCallback& onFail);

  void
  onQuerySuccess(const ndn::ConstBufferPtr& data,
                 const FaceUri& canonicalUri);

  void
  onQueryFailure(uint32_t errorCode,
                 const FaceUri& canonicalUri);

  void
  onCanonizeSuccess(const FaceUri& canonicalUri);

  void
  onCanonizeFailure(const std::string& reason);

  void
  startGetFaceId(const FaceUri& faceUri);

  void
  startFaceCreate(const FaceUri& canonicalUri);

  void
  onFaceCreateError(const ndn::nfd::ControlResponse& response,
                    const std::string& message);

  void
  succeed(uint32_t faceId);

  void
  fail(const std::string& reason);

private:
  ndn::Face& m_face;
  ndn::nfd::Controller& m_controller;
  bool m_allowCreate;
  SuccessCallback m_onSucceed;
  FailureCallback m_onFail;
  ndn::ValidatorNull m_validator;
};

} // namespace nfdc
} // namespace tools
} // namespace nfd

#endif // NFD_TOOLS_NFDC_FACE_ID_FETCHER_HPP
