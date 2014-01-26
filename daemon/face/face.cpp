/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "face.hpp"

namespace ndn {

Face::Face(FaceId id)
  : m_id(id)
{
}

Face::~Face()
{
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
Face::isLocalControlHeaderEnabled() const
{
  // TODO add local control header functionality
  return false;
}


} //namespace ndn
