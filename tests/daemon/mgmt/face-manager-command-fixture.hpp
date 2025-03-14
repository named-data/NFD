/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2025,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_MGMT_FACE_MANAGER_COMMAND_FIXTURE_HPP
#define NFD_TESTS_DAEMON_MGMT_FACE_MANAGER_COMMAND_FIXTURE_HPP

#include "mgmt/face-manager.hpp"
#include "fw/face-table.hpp"

#include "manager-common-fixture.hpp"

namespace nfd::tests {

class FaceManagerCommandNode
{
public:
  FaceManagerCommandNode(ndn::KeyChain& keyChain, uint16_t port);

  ~FaceManagerCommandNode();

  const Face*
  findFaceByUri(const std::string& uri) const;

  FaceId
  findFaceIdByUri(const std::string& uri) const;

public:
  ndn::DummyClientFace face;
  Dispatcher dispatcher;
  shared_ptr<CommandAuthenticator> authenticator;

  FaceTable faceTable;
  FaceSystem faceSystem;
  FaceManager manager;
};

class FaceManagerCommandFixture : public InterestSignerFixture
{
protected:
  FaceManagerCommandFixture();

  ~FaceManagerCommandFixture() override;

public:
  static inline const Name CREATE_REQUEST = Name("/localhost/nfd").append(ndn::nfd::FaceCreateCommand::getName());
  static inline const Name UPDATE_REQUEST = Name("/localhost/nfd").append(ndn::nfd::FaceUpdateCommand::getName());
  static inline const Name DESTROY_REQUEST = Name("/localhost/nfd").append(ndn::nfd::FaceDestroyCommand::getName());

protected:
  FaceManagerCommandNode node1; // used to test FaceManager
  FaceManagerCommandNode node2; // acts as a remote endpoint
};

} // namespace nfd::tests

#endif // NFD_TESTS_DAEMON_MGMT_FACE_MANAGER_COMMAND_FIXTURE_HPP
