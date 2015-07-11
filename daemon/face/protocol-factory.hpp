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

#ifndef NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP
#define NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP

#include "common.hpp"

namespace nfd {

class Channel;
class Face;

/**
 * \brief Prototype for the callback called when face is created
 *        (as a response to incoming connection or after connection
 *        is established)
 */
typedef function<void(const shared_ptr<Face>& newFace)> FaceCreatedCallback;

/**
 * \brief Prototype for the callback that is called when face is failed to
 *        get created
 */
typedef function<void(const std::string& reason)> FaceConnectFailedCallback;


class ProtocolFactory
{
public:
  /**
   * \brief Base class for all exceptions thrown by channel factories
   */
  struct Error : public std::runtime_error
  {
    Error(const std::string& what) : std::runtime_error(what) {}
  };

  /** \brief Try to create Face using the supplied FaceUri
   *
   * This method should automatically choose channel, based on supplied FaceUri
   * and create face.
   *
   * \throw Error Factory does not support connect operation
   * \throw Error specified \p persistency is not supported
   */
  virtual void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             const FaceCreatedCallback& onCreated,
             const FaceConnectFailedCallback& onConnectFailed) = 0;

  virtual std::list<shared_ptr<const Channel>>
  getChannels() const = 0;
};

} // namespace nfd

#endif // NFD_DAEMON_FACE_PROTOCOL_FACTORY_HPP
