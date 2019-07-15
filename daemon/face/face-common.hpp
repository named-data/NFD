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

#ifndef NFD_DAEMON_FACE_FACE_COMMON_HPP
#define NFD_DAEMON_FACE_FACE_COMMON_HPP

#include "core/common.hpp"
#include "common/logger.hpp"

#include <ndn-cxx/encoding/nfd-constants.hpp>

#include <boost/logic/tribool.hpp>

namespace nfd {
namespace face {

class Face;
class LinkService;

/** \brief Identifies a face.
 */
using FaceId = uint64_t;

/// indicates an invalid FaceId
const FaceId INVALID_FACEID = ndn::nfd::INVALID_FACE_ID;
/// identifies the InternalFace used in management
const FaceId FACEID_INTERNAL_FACE = 1;
/// identifies a packet comes from the ContentStore
const FaceId FACEID_CONTENT_STORE = 254;
/// identifies the NullFace that drops every packet
const FaceId FACEID_NULL = 255;
/// upper bound of reserved FaceIds
const FaceId FACEID_RESERVED_MAX = 255;

/** \brief Identifies a remote endpoint on the link.
 *
 *  This ID is only meaningful in the context of the same Transport.
 *  Incoming packets from the same remote endpoint have the same EndpointId,
 *  and incoming packets from different remote endpoints have different EndpointIds.
 *
 *  Typically, a point-to-point Transport has only one meaningful EndpointId (usually 0).
 */
using EndpointId = uint64_t;

/** \brief Parameters used to set Transport properties or LinkService options on a newly created face.
 *
 *  Parameters are passed as a struct rather than individually, so that a future change in the list
 *  of parameters does not require an update to the method signature in all subclasses.
 */
struct FaceParams
{
  ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT;
  optional<time::nanoseconds> baseCongestionMarkingInterval;
  optional<uint64_t> defaultCongestionThreshold;
  optional<ssize_t> mtu;
  bool wantLocalFields = false;
  bool wantLpReliability = false;
  boost::logic::tribool wantCongestionMarking = boost::logic::indeterminate;
};

/** \brief For internal use by FaceLogging macros.
 *
 *  FaceLogHelper wraps a reference to Face, LinkService, or Transport object.
 *
 *  `std::ostream& operator<<(std::ostream& os, const FaceLogHelper<T>& flh)`
 *  should be specialized to print "[id=888,local=scheme://local/uri,remote=scheme://remote/uri] "
 *  which will appear as part of the log message.
 */
template<typename T>
class FaceLogHelper
{
public:
  explicit
  FaceLogHelper(const T& obj1) noexcept
    : obj(obj1)
  {
  }

public:
  const T& obj;
};

} // namespace face

using face::EndpointId;
using face::FaceId;

} // namespace nfd

/** \defgroup FaceLogging Face logging macros
 *
 * These macros augment the log message with some face-specific information,
 * such as the face ID, that are useful to distinguish which face produced the
 * message. It is strongly recommended to use these macros instead of the
 * generic ones for all logging inside Face, LinkService, Transport subclasses.
 * @{
 */

/** \cond */
// implementation detail
#define NFD_LOG_FACE(level, msg) NFD_LOG_##level( \
  ::nfd::face::FaceLogHelper< \
    typename std::remove_cv< \
      typename std::remove_reference<decltype(*this)>::type \
    >::type \
  >(*this) \
  << msg)
/** \endcond */

/** \brief Log a message at TRACE level */
#define NFD_LOG_FACE_TRACE(msg) NFD_LOG_FACE(TRACE, msg)

/** \brief Log a message at DEBUG level */
#define NFD_LOG_FACE_DEBUG(msg) NFD_LOG_FACE(DEBUG, msg)

/** \brief Log a message at INFO level */
#define NFD_LOG_FACE_INFO(msg)  NFD_LOG_FACE(INFO,  msg)

/** \brief Log a message at WARN level */
#define NFD_LOG_FACE_WARN(msg)  NFD_LOG_FACE(WARN,  msg)

/** \brief Log a message at ERROR level */
#define NFD_LOG_FACE_ERROR(msg) NFD_LOG_FACE(ERROR, msg)

/** @} */

#endif // NFD_DAEMON_FACE_FACE_COMMON_HPP
