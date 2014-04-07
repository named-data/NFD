/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology
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
 **/

#include "face.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("Face")

static inline void
increaseCounter(FaceCounter& counter)
{
  ++counter;
}

Face::Face(const FaceUri& remoteUri, const FaceUri& localUri, bool isLocal)
  : m_id(INVALID_FACEID)
  , m_isLocal(isLocal)
  , m_remoteUri(remoteUri)
  , m_localUri(localUri)
  , m_isOnDemand(false)
{
  onReceiveInterest += bind(&increaseCounter, boost::ref(m_counters.getNInInterests()));
  onReceiveData     += bind(&increaseCounter, boost::ref(m_counters.getNInDatas()));
  onSendInterest    += bind(&increaseCounter, boost::ref(m_counters.getNOutInterests()));
  onSendData        += bind(&increaseCounter, boost::ref(m_counters.getNOutDatas()));
}

Face::~Face()
{
}

FaceId
Face::getId() const
{
  return m_id;
}

// this method is private and should be used only by the Forwarder
void
Face::setId(FaceId faceId)
{
  m_id = faceId;
}

void
Face::setDescription(const std::string& description)
{
  m_description = description;
}

const std::string&
Face::getDescription() const
{
  return m_description;
}

bool
Face::isMultiAccess() const
{
  return false;
}

bool
Face::isUp() const
{
  return true;
}

bool
Face::decodeAndDispatchInput(const Block& element)
{
  /// \todo Ensure lazy field decoding process

  if (element.type() == tlv::Interest)
    {
      shared_ptr<Interest> i = make_shared<Interest>();
      i->wireDecode(element);
      this->onReceiveInterest(*i);
    }
  else if (element.type() == tlv::Data)
    {
      shared_ptr<Data> d = make_shared<Data>();
      d->wireDecode(element);
      this->onReceiveData(*d);
    }
  else
    return false;

  return true;
}

} //namespace nfd
