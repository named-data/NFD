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

#include "multicast-discovery.hpp"

#include <ndn-cxx/util/segment-fetcher.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {

static const Name LOCALHOP_HUB_DISCOVERY_PREFIX = "/localhop/ndn-autoconf/hub";

MulticastDiscovery::MulticastDiscovery(Face& face, KeyChain& keyChain,
                                       const NextStageCallback& nextStageOnFailure)
  : Base(face, keyChain, nextStageOnFailure)
  , nRequestedRegs(0)
  , nFinishedRegs(0)
{
}

void
MulticastDiscovery::start()
{
  std::cerr << "Trying multicast discovery..." << std::endl;

  util::SegmentFetcher::fetch(m_face, Interest("/localhost/nfd/faces/list"),
                              ndn::util::DontVerifySegment(),
                              [this] (const ConstBufferPtr& data) {
                                registerHubDiscoveryPrefix(data);
                              },
                              [this] (uint32_t code, const std::string& msg) {
                                m_nextStageOnFailure(msg);
                              });
}

void
MulticastDiscovery::registerHubDiscoveryPrefix(const ConstBufferPtr& buffer)
{
  std::vector<uint64_t> multicastFaces;

  size_t offset = 0;
  while (offset < buffer->size()) {
    bool isOk = false;
    Block block;
    std::tie(isOk, block) = Block::fromBuffer(buffer, offset);
    if (!isOk) {
      std::cerr << "ERROR: cannot decode FaceStatus TLV" << std::endl;
      break;
    }

    offset += block.size();

    nfd::FaceStatus faceStatus(block);

    ndn::util::FaceUri uri(faceStatus.getRemoteUri());
    if (uri.getScheme() == "udp4") {
      namespace ip = boost::asio::ip;
      boost::system::error_code ec;
      ip::address address = ip::address::from_string(uri.getHost(), ec);

      if (!ec && address.is_multicast()) {
        multicastFaces.push_back(faceStatus.getFaceId());
      }
      else
        continue;
    }
  }

  if (multicastFaces.empty()) {
    m_nextStageOnFailure("No multicast faces available, skipping multicast discovery stage");
  }
  else {
    nfd::ControlParameters parameters;
    parameters
      .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
      .setCost(1)
      .setExpirationPeriod(time::seconds(30));

    nRequestedRegs = multicastFaces.size();
    nFinishedRegs = 0;

    for (const auto& face : multicastFaces) {
      parameters.setFaceId(face);
      m_controller.start<nfd::RibRegisterCommand>(parameters,
                                                  bind(&MulticastDiscovery::onRegisterSuccess,
                                                       this),
                                                  bind(&MulticastDiscovery::onRegisterFailure,
                                                       this, _1, _2));
    }
  }
}

void
MulticastDiscovery::onRegisterSuccess()
{
  ++nFinishedRegs;

  if (nRequestedRegs == nFinishedRegs) {
    MulticastDiscovery::setStrategy();
  }
}

void
MulticastDiscovery::onRegisterFailure(uint32_t code, const std::string& error)
{
  std::cerr << "ERROR: " << error << " (code: " << code << ")" << std::endl;
  --nRequestedRegs;

  if (nRequestedRegs == nFinishedRegs) {
    if (nRequestedRegs > 0) {
      MulticastDiscovery::setStrategy();
    } else {
      m_nextStageOnFailure("Failed to register " + LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() +
                           " for all multicast faces, skipping multicast discovery stage");
    }
  }
}

void
MulticastDiscovery::setStrategy()
{
  nfd::ControlParameters parameters;
  parameters
    .setName(LOCALHOP_HUB_DISCOVERY_PREFIX)
    .setStrategy("/localhost/nfd/strategy/multicast");

  m_controller.start<nfd::StrategyChoiceSetCommand>(parameters,
                                                    bind(&MulticastDiscovery::requestHubData, this),
                                                    bind(&MulticastDiscovery::onSetStrategyFailure,
                                                         this, _2));
}

void
MulticastDiscovery::onSetStrategyFailure(const std::string& error)
{
  m_nextStageOnFailure("Failed to set multicast strategy for " +
                       LOCALHOP_HUB_DISCOVERY_PREFIX.toUri() + " namespace (" + error + "). "
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
                         bind(m_nextStageOnFailure, "Timeout"));
}

void
MulticastDiscovery::onSuccess(Data& data)
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
