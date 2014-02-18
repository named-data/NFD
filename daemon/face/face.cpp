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
  , m_localControlHeaderFeatures(LOCAL_CONTROL_HEADER_FEATURE_MAX)
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
Face::isLocal() const
{
  return false;
}

bool
Face::isMultiAccess() const
{
  return false;
}

void
Face::setLocalControlHeaderFeature(LocalControlHeaderFeature feature, bool enabled)
{
  BOOST_ASSERT(feature > LOCAL_CONTROL_HEADER_FEATURE_ANY &&
               feature < m_localControlHeaderFeatures.size());
  m_localControlHeaderFeatures[feature] = enabled;
  NFD_LOG_DEBUG("face" << this->getId() << " setLocalControlHeaderFeature " <<
                (enabled ? "enable" : "disable") << " feature " << feature);

  BOOST_STATIC_ASSERT(LOCAL_CONTROL_HEADER_FEATURE_ANY == 0);
  m_localControlHeaderFeatures[LOCAL_CONTROL_HEADER_FEATURE_ANY] =
    std::find(m_localControlHeaderFeatures.begin() + 1,
              m_localControlHeaderFeatures.end(), true) !=
              m_localControlHeaderFeatures.end();
}

} //namespace nfd
