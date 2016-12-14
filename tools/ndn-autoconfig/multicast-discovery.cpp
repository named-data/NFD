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

#include "multicast-discovery.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

static const Name LOCALHOP_HUB_DISCOVERY_PREFIX = "/localhop/ndn-autoconf/hub";

MulticastDiscovery::MulticastDiscovery(Face& face, KeyChain& keyChain,
                                       const NextStageCallback& nextStageOnFailure)
  : Base(face, keyChain, nextStageOnFailure)
  , m_nRequestedRegs(0)
  , m_nFinishedRegs(0)
{
}

void
MulticastDiscovery::start()
{
  std::cerr << "Trying multicast discovery..." << std::endl;

  this->collectMulticastFaces();
}

void
MulticastDiscovery::collectMulticastFaces()
{
  ndn::nfd::FaceQueryFilter filter;
  filter.setLinkType(ndn::nfd::LINK_TYPE_MULTI_ACCESS);
  m_controller.fetch<ndn::nfd::FaceQueryDataset>(
    filter,
    bind(&MulticastDiscovery::registerHubDiscoveryPrefix, this, _1),
    bind(m_nextStageOnFailure, _2)
  );
}

void
MulticastDiscovery::registerHubDiscoveryPrefix(const std::vector<ndn::nfd::FaceStatus>& dataset)
{
  std::vector<uint64_t> multicastFaces;
  std::transform(dataset.begin(), dataset.end(), std::back_inserter(multicastFaces),
                 [] (const ndn::nfd::FaceStatus& faceStatus) { return faceStatus.getFaceId(); });

  if (multicastFaces.empty()) {
    m_nextStageOnFailure("No multicast faces available, skipping multicast discovery stage");
  }
  else {
    ControlParameters parameters;
    parameters
      .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
      .setCost(1)
      .setExpirationPeriod(time::seconds(30));

    m_nRequestedRegs = multicastFaces.size();
    m_nFinishedRegs = 0;

    for (const auto& face : multicastFaces) {
      parameters.setFaceId(face);
      m_controller.start<ndn::nfd::RibRegisterCommand>(
        parameters,
        bind(&MulticastDiscovery::onRegisterSuccess, this),
        bind(&MulticastDiscovery::onRegisterFailure, this, _1));
    }
  }
}

void
MulticastDiscovery::onRegisterSuccess()
{
  ++m_nFinishedRegs;

  if (m_nRequestedRegs == m_nFinishedRegs) {
    MulticastDiscovery::setStrategy();
  }
}

void
MulticastDiscovery::onRegisterFailure(const ControlResponse& response)
{
  std::cerr << "ERROR: " << response.getText() << " (code: " << response.getCode() << ")" << std::endl;
  --m_nRequestedRegs;

  if (m_nRequestedRegs == m_nFinishedRegs) {
    if (m_nRequestedRegs > 0) {
      MulticastDiscovery::setStrategy();
    }
    else {
      m_nextStageOnFailure("Failed to register " + LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() +
                           " for all multicast faces, skipping multicast discovery stage");
    }
  }
}

void
MulticastDiscovery::setStrategy()
{
  ControlParameters parameters;
  parameters
    .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
    .setStrategy("/localhost/nfd/strategy/multicast");

  m_controller.start<ndn::nfd::StrategyChoiceSetCommand>(
    parameters,
    bind(&MulticastDiscovery::requestHubData, this),
    bind(&MulticastDiscovery::onSetStrategyFailure, this, _1));
}

void
MulticastDiscovery::onSetStrategyFailure(const ControlResponse& response)
{
  m_nextStageOnFailure("Failed to set multicast strategy for " +
                       LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() + " namespace (" + response.getText() + "). "
                       "Skipping multicast discovery stage");
}

void
MulticastDiscovery::requestHubData()
{
  Interest interest(LOCALHOP_HUB_DISCOVERY_PREFIX);
  interest.setInterestLifetime(time::milliseconds(4000)); // 4 seconds
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
                         bind(&MulticastDiscovery::onSuccess, this, _2),
                         bind(m_nextStageOnFailure, "HUB Data not received: nacked"),
                         bind(m_nextStageOnFailure, "HUB Data not received: timeout"));
}

void
MulticastDiscovery::onSuccess(const Data& data)
{
  const Block& content = data.getContent();
  content.parse();

  // Get Uri
  Block::element_const_iterator blockValue = content.find(tlv::nfd::Uri);
  if (blockValue == content.elements_end()) {
    m_nextStageOnFailure("Incorrect reply to multicast discovery stage");
    return;
  }
  std::string hubUri(reinterpret_cast<const char*>(blockValue->value()), blockValue->value_size());
  this->connectToHub(hubUri);
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn
