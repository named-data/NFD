/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_DAEMON_FACE_FACE_HPP
#define NFD_DAEMON_FACE_FACE_HPP

#include "common.hpp"
#include "face-counters.hpp"

#include <ndn-cxx/util/face-uri.hpp>
#include <ndn-cxx/management/nfd-face-status.hpp>
#include <ndn-cxx/management/nfd-face-event-notification.hpp>

namespace nfd {

/** \class FaceId
 *  \brief identifies a face
 */
typedef int FaceId;

/// indicates an invalid FaceId
const FaceId INVALID_FACEID = -1;

/// identifies the InternalFace used in management
const FaceId FACEID_INTERNAL_FACE = 1;
/// identifies a packet comes from the ContentStore, in LocalControlHeader incomingFaceId
const FaceId FACEID_CONTENT_STORE = 254;
/// identifies the NullFace that drops every packet
const FaceId FACEID_NULL = 255;
/// upper bound of reserved FaceIds
const FaceId FACEID_RESERVED_MAX = 255;

using ndn::util::FaceUri;

/** \brief represents a face
 */
class Face : noncopyable, public enable_shared_from_this<Face>
{
public:
  /**
   * \brief Face-related error
   */
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

  Face(const FaceUri& remoteUri, const FaceUri& localUri, bool isLocal = false);

  virtual
  ~Face();

  /// fires when an Interest is received
  EventEmitter<Interest> onReceiveInterest;

  /// fires when a Data is received
  EventEmitter<Data> onReceiveData;

  /// fires when an Interest is sent out
  EventEmitter<Interest> onSendInterest;

  /// fires when a Data is sent out
  EventEmitter<Data> onSendData;

  /// fires when face disconnects or fails to perform properly
  EventEmitter<std::string/*reason*/> onFail;

  /// send an Interest
  virtual void
  sendInterest(const Interest& interest) = 0;

  /// send a Data
  virtual void
  sendData(const Data& data) = 0;

  /** \brief Close the face
   *
   *  This terminates all communication on the face and cause
   *  onFail() method event to be invoked
   */
  virtual void
  close() = 0;

public: // attributes
  FaceId
  getId() const;

  /** \brief Set the description
   *
   *  This is typically invoked by mgmt on set description command
   */
  virtual void
  setDescription(const std::string& description);

  /// Get the description
  virtual const std::string&
  getDescription() const;

  /** \brief Get whether face is connected to a local app
   */
  bool
  isLocal() const;

  /** \brief Get whether packets sent this Face may reach multiple peers
   *
   *  In this base class this property is always false.
   */
  virtual bool
  isMultiAccess() const;

  /** \brief Get whether underlying communication is up
   *
   *  In this base class this property is always true.
   */
  virtual bool
  isUp() const;

  /** \brief Get whether face is created on demand or explicitly via FaceManagement protocol
   */
  bool
  isOnDemand() const;

  const FaceCounters&
  getCounters() const;

  /** \return a FaceUri that represents the remote endpoint
   */
  const FaceUri&
  getRemoteUri() const;

  /** \return a FaceUri that represents the local endpoint (NFD side)
   */
  const FaceUri&
  getLocalUri() const;

  /** \return FaceTraits data structure filled with the current FaceTraits status
   */
  template<typename FaceTraits>
  void
  copyStatusTo(FaceTraits& traits) const;

  /** \return FaceStatus data structure filled with the current Face status
   */
  virtual ndn::nfd::FaceStatus
  getFaceStatus() const;

protected:
  // this is a non-virtual method
  bool
  decodeAndDispatchInput(const Block& element);

  FaceCounters&
  getMutableCounters();

  void
  setOnDemand(bool isOnDemand);

  /** \brief fail the face and raise onFail event if it's UP; otherwise do nothing
   */
  void
  fail(const std::string& reason);

private:
  void
  setId(FaceId faceId);

private:
  FaceId m_id;
  std::string m_description;
  bool m_isLocal; // for scoping purposes
  FaceCounters m_counters;
  FaceUri m_remoteUri;
  FaceUri m_localUri;
  bool m_isOnDemand;
  bool m_isFailed;

  // allow setting FaceId
  friend class FaceTable;
};

inline bool
Face::isLocal() const
{
  return m_isLocal;
}

inline const FaceCounters&
Face::getCounters() const
{
  return m_counters;
}

inline FaceCounters&
Face::getMutableCounters()
{
  return m_counters;
}

inline const FaceUri&
Face::getRemoteUri() const
{
  return m_remoteUri;
}

inline const FaceUri&
Face::getLocalUri() const
{
  return m_localUri;
}

inline void
Face::setOnDemand(bool isOnDemand)
{
  m_isOnDemand = isOnDemand;
}

inline bool
Face::isOnDemand() const
{
  return m_isOnDemand;
}

} // namespace nfd

#endif // NFD_DAEMON_FACE_FACE_HPP
