/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face.hpp"
#include "core/logger.hpp"

namespace nfd {

NFD_LOG_INIT("Face");

Face::Face()
  : m_id(INVALID_FACEID)
{
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

bool
Face::isLocal() const
{
  return m_isLocal;
}

// this method is protected and can be used only in derived class
// to set localhost scope
void
Face::setLocal(bool isLocal)
{
  m_isLocal = isLocal;
}

bool
Face::isUp() const
{
  return true;
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
