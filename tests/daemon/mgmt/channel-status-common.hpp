/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

#ifndef NFD_TESTS_NFD_MGMT_CHANNEL_STATUS_COMMON_HPP
#define NFD_TESTS_NFD_MGMT_CHANNEL_STATUS_COMMON_HPP

#include "face/protocol-factory.hpp"
#include "face/channel.hpp"

#include "tests/test-common.hpp"

#include <ndn-cxx/management/nfd-channel-status.hpp>



namespace nfd {
namespace tests {

class DummyChannel : public Channel
{
public:

  DummyChannel(const std::string& uri)
  {
    setUri(FaceUri(uri));
  }

  virtual
  ~DummyChannel()
  {
  }
};

class DummyProtocolFactory : public ProtocolFactory
{
public:

  DummyProtocolFactory()
  {

  }

  virtual void
  createFace(const FaceUri& uri,
             ndn::nfd::FacePersistency persistency,
             const FaceCreatedCallback& onCreated,
             const FaceConnectFailedCallback& onConnectFailed)
  {
  }

  virtual void
  addChannel(const std::string& channelUri)
  {
    m_channels.push_back(make_shared<DummyChannel>(channelUri));
  }

  virtual std::list<shared_ptr<const Channel> >
  getChannels() const
  {
    return m_channels;
  }

  virtual size_t
  getNChannels() const
  {
    return m_channels.size();
  }

private:
  std::list<shared_ptr<const Channel> > m_channels;
};

} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_NFD_MGMT_CHANNEL_STATUS_COMMON_HPP
