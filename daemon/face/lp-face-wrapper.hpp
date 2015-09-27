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

#ifndef NFD_DAEMON_FACE_LP_FACE_WRAPPER_HPP
#define NFD_DAEMON_FACE_LP_FACE_WRAPPER_HPP

#include "common.hpp"
#include "face.hpp"
#include "lp-face.hpp"

namespace nfd {
namespace face {

/** \brief an adaptor to provide old Face APIs from an LpFace
 *  \sa LpFace
 *  \note When LpFace is adapted by LpFaceWrapper,
 *        FaceId and counters will come from old Face rather than LpFace.
 */
class LpFaceWrapper : public Face
{
public:
  explicit
  LpFaceWrapper(unique_ptr<LpFace> face);

  LpFace*
  getLpFace();

  virtual void
  sendInterest(const Interest& interest) DECL_OVERRIDE;

  virtual void
  sendData(const Data& data) DECL_OVERRIDE;

  virtual void
  sendNack(const lp::Nack& nack) DECL_OVERRIDE;

  virtual void
  close() DECL_OVERRIDE;

  virtual bool
  isUp() const DECL_OVERRIDE;

  virtual void
  setPersistency(ndn::nfd::FacePersistency persistency) DECL_OVERRIDE;

  virtual const FaceCounters&
  getCounters() const DECL_OVERRIDE;

protected:
  virtual void
  setId(nfd::FaceId faceId) DECL_OVERRIDE;

private:
  void
  dispatchInterest(const Interest& interest);

  void
  dispatchData(const Data& data);

  void
  dispatchNack(const lp::Nack& nack);

  void
  handleStateChange(FaceState oldState, FaceState newState);

private:
  unique_ptr<LpFace> m_face;
};

inline LpFace*
LpFaceWrapper::getLpFace()
{
  return m_face.get();
}

inline void
LpFaceWrapper::sendInterest(const Interest& interest)
{
  this->emitSignal(onSendInterest, interest);
  m_face->sendInterest(interest);
}

inline void
LpFaceWrapper::sendData(const Data& data)
{
  this->emitSignal(onSendData, data);
  m_face->sendData(data);
}

inline void
LpFaceWrapper::sendNack(const lp::Nack& nack)
{
  this->emitSignal(onSendNack, nack);
  m_face->sendNack(nack);
}

inline void
LpFaceWrapper::close()
{
  m_face->close();
}

inline bool
LpFaceWrapper::isUp() const
{
  return m_face->getState() == FaceState::UP;
}

inline const FaceCounters&
LpFaceWrapper::getCounters() const
{
  return m_face->getCounters();
}

} // namespace face
} // namespace nfd

#endif // NFD_DAEMON_FACE_LP_FACE_WRAPPER_HPP
