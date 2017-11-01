/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2017,  Regents of the University of California,
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

#include "face.hpp"

namespace nfd {
namespace face {

Face::Face(unique_ptr<LinkService> service, unique_ptr<Transport> transport)
  : afterReceiveInterest(service->afterReceiveInterest)
  , afterReceiveData(service->afterReceiveData)
  , afterReceiveNack(service->afterReceiveNack)
  , onDroppedInterest(service->onDroppedInterest)
  , afterStateChange(transport->afterStateChange)
  , m_id(INVALID_FACEID)
  , m_service(std::move(service))
  , m_transport(std::move(transport))
  , m_counters(m_service->getCounters(), m_transport->getCounters())
{
  m_service->setFaceAndTransport(*this, *m_transport);
  m_transport->setFaceAndLinkService(*this, *m_service);
}

std::ostream&
operator<<(std::ostream& os, const FaceLogHelper<Face>& flh)
{
  const Face& face = flh.obj;
  os << "[id=" << face.getId() << ",local=" << face.getLocalUri() <<
        ",remote=" << face.getRemoteUri() << "] ";
  return os;
}

} // namespace face
} // namespace nfd
