/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_RECEIVE_LINK_SERVICE_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_RECEIVE_LINK_SERVICE_HPP

#include "core/common.hpp"

#include "face/link-service.hpp"

namespace nfd {
namespace face {
namespace tests {

/** \brief a dummy LinkService that logs all received packets, for Transport testing
 *  \warning This LinkService does not allow sending.
 */
class DummyReceiveLinkService : public LinkService
{
private:
  virtual void
  doSendInterest(const Interest& interest) override
  {
    BOOST_ASSERT(false);
  }

  virtual void
  doSendData(const Data& data) override
  {
    BOOST_ASSERT(false);
  }

  virtual void
  doSendNack(const lp::Nack& nack) override
  {
    BOOST_ASSERT(false);
  }

  virtual void
  doReceivePacket(Transport::Packet&& packet) override
  {
    receivedPackets.push_back(packet);
  }

public:
  std::vector<Transport::Packet> receivedPackets;
};

} // namespace tests
} // namespace face
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_RECEIVE_LINK_SERVICE_HPP
