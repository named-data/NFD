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

#include "lp-face-wrapper.hpp"

namespace nfd {
namespace face {

LpFaceWrapper::LpFaceWrapper(unique_ptr<LpFace> face)
  : Face(face->getRemoteUri(), face->getLocalUri(),
         face->getScope() == ndn::nfd::FACE_SCOPE_LOCAL,
         face->getLinkType() == ndn::nfd::LINK_TYPE_MULTI_ACCESS)
  , m_face(std::move(face))
{
  this->Face::setPersistency(m_face->getPersistency());
  m_face->afterReceiveInterest.connect(bind(&LpFaceWrapper::dispatchInterest, this, _1));
  m_face->afterReceiveData.connect(bind(&LpFaceWrapper::dispatchData, this, _1));
  m_face->afterReceiveNack.connect(bind(&LpFaceWrapper::dispatchNack, this, _1));
  m_face->afterStateChange.connect(bind(&LpFaceWrapper::handleStateChange, this, _1, _2));
}

void
LpFaceWrapper::setPersistency(ndn::nfd::FacePersistency persistency)
{
  this->Face::setPersistency(persistency);
  m_face->setPersistency(persistency);
}

void
LpFaceWrapper::setId(nfd::FaceId faceId)
{
  this->Face::setId(faceId);

  FaceId lpId = static_cast<FaceId>(faceId);
  if (faceId == nfd::INVALID_FACEID) {
    lpId = INVALID_FACEID;
  }
  m_face->setId(lpId);
}

void
LpFaceWrapper::dispatchInterest(const Interest& interest)
{
  this->emitSignal(onReceiveInterest, interest);
}

void
LpFaceWrapper::dispatchData(const Data& data)
{
  this->emitSignal(onReceiveData, data);
}

void
LpFaceWrapper::dispatchNack(const ndn::lp::Nack& nack)
{
  this->emitSignal(onReceiveNack, nack);
}

void
LpFaceWrapper::handleStateChange(FaceState oldState, FaceState newState)
{
  if (newState != FaceState::CLOSED) {
    return;
  }

  switch (oldState) {
  case FaceState::CLOSING:
    this->fail("LpFace closed");
    break;
  case FaceState::FAILED:
    this->fail("LpFace failed");
    break;
  default:
    BOOST_ASSERT(false);
    break;
  }
}

} // namespace face
} // namespace nfd
