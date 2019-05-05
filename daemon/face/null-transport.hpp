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

#ifndef NFD_DAEMON_FACE_NULL_TRANSPORT_HPP
#define NFD_DAEMON_FACE_NULL_TRANSPORT_HPP

#include "transport.hpp"

namespace nfd {
namespace face {

/** \brief A Transport that drops every packet.
 */
class NullTransport FINAL_UNLESS_WITH_TESTS : public Transport
{
public:
  explicit
  NullTransport(const FaceUri& localUri = FaceUri("null://"),
                const FaceUri& remoteUri = FaceUri("null://"),
                ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL,
                ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERMANENT);

protected:
  void
  doClose() OVERRIDE_WITH_TESTS_ELSE_FINAL
  {
    setState(TransportState::CLOSED);
  }

private:
  void
  doSend(const Block&, const EndpointId&) OVERRIDE_WITH_TESTS_ELSE_FINAL
  {
  }
};

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_NULL_TRANSPORT_HPP
