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

#include "stage.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

void
Stage::start()
{
  if (m_isInProgress) {
    BOOST_THROW_EXCEPTION(Error("Cannot start a stage when it's in progress"));
  }
  m_isInProgress = true;

  std::cerr << "Starting " << this->getName() << " stage" << std::endl;
  this->doStart();
}

void
Stage::provideHubFaceUri(const std::string& s)
{
  FaceUri u;
  if (u.parse(s)) {
    this->succeed(u);
  }
  else {
    this->fail("Cannot parse FaceUri: " + s);
  }
}

void
Stage::succeed(const FaceUri& hubFaceUri)
{
  std::cerr << "Stage " << this->getName() << " succeeded with " << hubFaceUri << std::endl;
  this->onSuccess(hubFaceUri);
  m_isInProgress = false;
}

void
Stage::fail(const std::string& msg)
{
  std::cerr << "Stage " << this->getName() << " failed: " << msg << std::endl;
  this->onFailure(msg);
  m_isInProgress = false;
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn
