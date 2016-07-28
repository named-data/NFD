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

#ifndef NFD_TESTS_DAEMON_FACE_DUMMY_FACE_HPP
#define NFD_TESTS_DAEMON_FACE_DUMMY_FACE_HPP

#include "face/face.hpp"

namespace nfd {
namespace face {
namespace tests {

class DummyTransport;

/** \brief a Face for unit testing
 *
 *  The DummyFace has no underlying transport, but allows observing outgoing packets
 *  and injecting incoming packets at network layer.
 *  It's primarily used for forwarding test suites, but can be used in other tests as well.
 *
 *  Outgoing network-layer packets sent through the DummyFace are recorded in sent* vectors,
 *  which can be observed in test cases.
 *  Incoming network-layer packets can be injected from test cases through receive* method.
 */
class DummyFace : public Face
{
public:
  class LinkService;

  DummyFace(const std::string& localUri = "dummy://", const std::string& remoteUri = "dummy://",
            ndn::nfd::FaceScope scope = ndn::nfd::FACE_SCOPE_NON_LOCAL,
            ndn::nfd::FacePersistency persistency = ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
            ndn::nfd::LinkType linkType = ndn::nfd::LINK_TYPE_POINT_TO_POINT);

  /** \brief changes face state
   *  \throw std::runtime_error state transition is invalid
   */
  void
  setState(FaceState state);

  /** \brief causes the face to receive an Interest
   */
  void
  receiveInterest(const Interest& interest);

  /** \brief causes the face to receive a Data
   */
  void
  receiveData(const Data& data);

  /** \brief causes the face to receive a Nack
   */
  void
  receiveNack(const lp::Nack& nack);

  /** \brief signals after any network-layer packet is sent
   *
   *  The network-layer packet type is indicated as an argument,
   *  which is either of tlv::Interest, tlv::Data, or lp::tlv::Nack.
   *  The callback may retrieve the packet from sentInterests.back(), sentData.back(), or sentNacks.back().
   */
  signal::Signal<LinkService, uint32_t>& afterSend;

private:
  LinkService*
  getLinkServiceInternal();

  DummyTransport*
  getTransportInternal();

public:
  std::vector<Interest>& sentInterests;
  std::vector<Data>& sentData;
  std::vector<lp::Nack>& sentNacks;
};

} // namespace tests
} // namespace face

namespace tests {
using nfd::face::tests::DummyFace;
} // namespace tests
} // namespace nfd

#endif // NFD_TESTS_DAEMON_FACE_DUMMY_FACE_HPP
