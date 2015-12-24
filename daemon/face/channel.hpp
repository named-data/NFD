/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#include "face.hpp"

namespace nfd {

/**
 * \brief Prototype for the callback that is invoked when the face
 *        is created (as a response to incoming connection or after
 *        connection is established)
 */
typedef function<void(const shared_ptr<Face>& newFace)> FaceCreatedCallback;

/**
 * \brief Prototype for the callback that is invoked when the face
 *        fails to be created
 */
typedef function<void(const std::string& reason)> FaceCreationFailedCallback;


class Channel : noncopyable
{
public:
  virtual
  ~Channel();

  const FaceUri&
  getUri() const;

protected:
  void
  setUri(const FaceUri& uri);

private:
  FaceUri m_uri;
};

inline const FaceUri&
Channel::getUri() const
{
  return m_uri;
}

/** \brief invokes a callback when the face is closed
 *  \param face the face
 *  \param f the callback to be invoked when the face enters CLOSED state
 *
 *  This function connects a callback to the afterStateChange signal on the \p face,
 *  and invokes \p f when the state becomes CLOSED.
 */
void
connectFaceClosedSignal(Face& face, const std::function<void()>& f);

} // namespace nfd

#endif // NFD_DAEMON_FACE_CHANNEL_HPP
