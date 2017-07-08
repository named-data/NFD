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

#include "multicast-discovery.hpp"
#include <boost/lexical_cast.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>

namespace ndn {
namespace tools {
namespace autoconfig {

using nfd::ControlParameters;
using nfd::ControlResponse;

static const Name HUB_DISCOVERY_PREFIX("/localhop/ndn-autoconf/hub");
static const uint64_t HUB_DISCOVERY_ROUTE_COST(1);
static const time::milliseconds HUB_DISCOVERY_ROUTE_EXPIRATION = time::seconds(30);
static const time::milliseconds HUB_DISCOVERY_INTEREST_LIFETIME = time::seconds(4);

MulticastDiscovery::MulticastDiscovery(Face& face, nfd::Controller& controller)
  : m_face(face)
  , m_controller(controller)
{
}

void
MulticastDiscovery::doStart()
{
  nfd::FaceQueryFilter filter;
  filter.setLinkType(nfd::LINK_TYPE_MULTI_ACCESS);

  m_controller.fetch<nfd::FaceQueryDataset>(
    filter,
    bind(&MulticastDiscovery::registerHubDiscoveryPrefix, this, _1),
    [this] (uint32_t code, const std::string& reason) {
      this->fail("Error " + to_string(code) + " when querying multi-access faces: " + reason);
    });
}

void
MulticastDiscovery::registerHubDiscoveryPrefix(const std::vector<nfd::FaceStatus>& dataset)
{
  if (dataset.empty()) {
    this->fail("No multi-access faces available");
    return;
  }

  m_nRegs = dataset.size();
  m_nRegSuccess = 0;
  m_nRegFailure = 0;

  for (const auto& faceStatus : dataset) {
    ControlParameters parameters;
    parameters.setName(HUB_DISCOVERY_PREFIX)
              .setFaceId(faceStatus.getFaceId())
              .setCost(HUB_DISCOVERY_ROUTE_COST)
              .setExpirationPeriod(HUB_DISCOVERY_ROUTE_EXPIRATION);

    m_controller.start<nfd::RibRegisterCommand>(
      parameters,
      [this] (const ControlParameters&) {
        ++m_nRegSuccess;
        afterReg();
      },
      [this, faceStatus] (const ControlResponse& resp) {
        std::cerr << "Error " << resp.getCode() << " when registering hub discovery prefix "
                  << "for face " << faceStatus.getFaceId() << " (" << faceStatus.getRemoteUri()
                  << "): " << resp.getText() << std::endl;
        ++m_nRegFailure;
        afterReg();
      });
  }
}

void
MulticastDiscovery::afterReg()
{
  if (m_nRegSuccess + m_nRegFailure < m_nRegs) {
    return; // continue waiting
  }
  if (m_nRegSuccess > 0) {
    this->setStrategy();
  }
  else {
    this->fail("Cannot register hub discovery prefix for any face");
  }
}

void
MulticastDiscovery::setStrategy()
{
  ControlParameters parameters;
  parameters.setName(HUB_DISCOVERY_PREFIX)
            .setStrategy("/localhost/nfd/strategy/multicast"),

  m_controller.start<nfd::StrategyChoiceSetCommand>(
    parameters,
    bind(&MulticastDiscovery::requestHubData, this),
    [this] (const ControlResponse& resp) {
      this->fail("Error " + to_string(resp.getCode()) + " when setting multicast strategy: " +
                 resp.getText());
    });
}

void
MulticastDiscovery::requestHubData()
{
  Interest interest(HUB_DISCOVERY_PREFIX);
  interest.setInterestLifetime(HUB_DISCOVERY_INTEREST_LIFETIME);
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
    [this] (const Interest&, const Data& data) {
      const Block& content = data.getContent();
      content.parse();

      auto i = content.find(tlv::nfd::Uri);
      if (i == content.elements_end()) {
        this->fail("Malformed hub Data: missing Uri element");
        return;
      }

      this->provideHubFaceUri(std::string(reinterpret_cast<const char*>(i->value()), i->value_size()));
    },
    [this] (const Interest&, const lp::Nack& nack) {
      this->fail("Nack-" + boost::lexical_cast<std::string>(nack.getReason()) + " when retrieving hub Data");
    },
    [this] (const Interest&) {
      this->fail("Timeout when retrieving hub Data");
    });
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn
