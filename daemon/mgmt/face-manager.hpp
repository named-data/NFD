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

#ifndef NFD_DAEMON_MGMT_FACE_MANAGER_HPP
#define NFD_DAEMON_MGMT_FACE_MANAGER_HPP

#include "manager-base.hpp"
#include "face/face.hpp"
#include "face/face-system.hpp"

namespace nfd {

/**
 * @brief Implements the Face Management of NFD Management Protocol.
 * @sa https://redmine.named-data.net/projects/nfd/wiki/FaceMgmt
 */
class FaceManager : public ManagerBase
{
public:
  FaceManager(FaceSystem& faceSystem,
              Dispatcher& dispatcher, CommandAuthenticator& authenticator);

private: // ControlCommand
  void
  createFace(const ControlParameters& parameters,
             const ndn::mgmt::CommandContinuation& done);

  void
  updateFace(const Interest& interest,
             const ControlParameters& parameters,
             const ndn::mgmt::CommandContinuation& done);

  void
  destroyFace(const ControlParameters& parameters,
              const ndn::mgmt::CommandContinuation& done);

private: // helpers for ControlCommand
  void
  afterCreateFaceSuccess(const shared_ptr<Face>& face,
                         const ControlParameters& parameters,
                         const ndn::mgmt::CommandContinuation& done);

private: // StatusDataset
  void
  listFaces(ndn::mgmt::StatusDatasetContext& context);

  void
  listChannels(ndn::mgmt::StatusDatasetContext& context);

  void
  queryFaces(const Interest& interest, ndn::mgmt::StatusDatasetContext& context);

private: // NotificationStream
  void
  notifyFaceEvent(const Face& face, ndn::nfd::FaceEventKind kind);

  void
  connectFaceStateChangeSignal(const Face& face);

private:
  FaceSystem& m_faceSystem;
  FaceTable& m_faceTable;
  ndn::mgmt::PostNotification m_postNotification;
  signal::ScopedConnection m_faceAddConn;
  signal::ScopedConnection m_faceRemoveConn;

PUBLIC_WITH_TESTS_ELSE_PRIVATE:
  std::map<FaceId, signal::ScopedConnection> m_faceStateChangeConn;
};

} // namespace nfd

#endif // NFD_DAEMON_MGMT_FACE_MANAGER_HPP
