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

#ifndef NFD_DAEMON_FACE_UNIX_STREAM_FACTORY_HPP
#define NFD_DAEMON_FACE_UNIX_STREAM_FACTORY_HPP

#include "protocol-factory.hpp"
#include "unix-stream-channel.hpp"

namespace nfd {
namespace face {

/** \brief protocol factory for stream-oriented Unix sockets
 */
class UnixStreamFactory : public ProtocolFactory
{
public:
  static const std::string&
  getId();

  explicit
  UnixStreamFactory(const CtorParams& params);

  /** \brief process face_system.unix config section
   */
  void
  processConfig(OptionalConfigSection configSection,
                FaceSystem::ConfigContext& context) override;

  void
  createFace(const CreateFaceRequest& req,
             const FaceCreatedCallback& onCreated,
             const FaceCreationFailedCallback& onFailure) override;

  /**
   * \brief Create stream-oriented Unix channel using specified socket path
   *
   * If this method is called twice with the same path, only one channel
   * will be created.  The second call will just retrieve the existing
   * channel.
   *
   * \returns always a valid pointer to a UnixStreamChannel object,
   *          an exception will be thrown if the channel cannot be created.
   */
  shared_ptr<UnixStreamChannel>
  createChannel(const std::string& unixSocketPath);

  std::vector<shared_ptr<const Channel>>
  getChannels() const override;

private:
  /**
   * \brief Look up UnixStreamChannel using specified endpoint
   *
   * \returns shared pointer to the existing UnixStreamChannel object
   *          or empty shared pointer when such channel does not exist
   */
  shared_ptr<UnixStreamChannel>
  findChannel(const unix_stream::Endpoint& endpoint) const;

private:
  bool m_wantCongestionMarking = false;
  std::map<unix_stream::Endpoint, shared_ptr<UnixStreamChannel>> m_channels;
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_UNIX_STREAM_FACTORY_HPP
