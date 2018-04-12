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

#ifndef NFD_DAEMON_FACE_CHANNEL_HPP
#define NFD_DAEMON_FACE_CHANNEL_HPP

#include "channel-log.hpp"
#include "face.hpp"

#include <boost/logic/tribool.hpp>

namespace nfd {
namespace face {

/** \brief Prototype for the callback that is invoked when a face is created
 *         (in response to an incoming connection or after a connection is established)
 */
using FaceCreatedCallback = std::function<void(const shared_ptr<Face>& face)>;

/** \brief Prototype for the callback that is invoked when a face fails to be created
 */
using FaceCreationFailedCallback = std::function<void(uint32_t status, const std::string& reason)>;

/** \brief represent a channel that communicates on a local endpoint
 *  \sa FaceSystem
 *
 *  A channel can listen on a local endpoint and initiate outgoing connection from a local endpoint.
 *  A channel creates Face objects and retains shared ownership of them.
 */
class Channel : noncopyable
{
public:
  virtual
  ~Channel();

  const FaceUri&
  getUri() const
  {
    return m_uri;
  }

  /** \brief Returns whether the channel is listening
   */
  virtual bool
  isListening() const = 0;

  /** \brief Returns the number of faces in the channel
   */
  virtual size_t
  size() const = 0;

protected:
  void
  setUri(const FaceUri& uri);

private:
  FaceUri m_uri;
};

/** \brief Parameters used to set Transport properties or LinkService options on a newly created face
 *
 *  Parameters are passed as a struct rather than individually, so that a future change in the list
 *  of parameters does not require an update to the method signature in all subclasses.
 */
class FaceParams
{
public:
  // get rid of this constructor and use aggregate init + NSDMIs when we switch to C++14
  FaceParams() noexcept
    : persistency(ndn::nfd::FACE_PERSISTENCY_PERSISTENT)
    , wantLocalFields(false)
    , wantLpReliability(false)
    , wantCongestionMarking(boost::logic::indeterminate)
  {
  }

public:
  ndn::nfd::FacePersistency persistency;
  optional<time::nanoseconds> baseCongestionMarkingInterval;
  optional<uint64_t> defaultCongestionThreshold;
  bool wantLocalFields;
  bool wantLpReliability;
  boost::logic::tribool wantCongestionMarking;
};

/** \brief invokes a callback when the face is closed
 *  \param face the face
 *  \param f the callback to be invoked when the face enters CLOSED state
 *
 *  This function connects a callback to the afterStateChange signal on the \p face,
 *  and invokes \p f when the state becomes CLOSED.
 */
void
connectFaceClosedSignal(Face& face, const std::function<void()>& f);

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_CHANNEL_HPP
