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

#include "procedure.hpp"
#include "guess-from-identity-name.hpp"
#include "guess-from-search-domains.hpp"
#include "multicast-discovery.hpp"
#include "ndn-fch-discovery.hpp"

namespace ndn {
namespace tools {
namespace autoconfig {

using nfd::ControlParameters;
using nfd::ControlResponse;

static const time::nanoseconds FACEURI_CANONIZE_TIMEOUT = time::seconds(4);
static const std::vector<Name> HUB_PREFIXES{"/", "/localhop/nfd"};
static const nfd::RouteOrigin HUB_ROUTE_ORIGIN = nfd::ROUTE_ORIGIN_AUTOCONF;
static const uint64_t HUB_ROUTE_COST = 100;

Procedure::Procedure(Face& face, KeyChain& keyChain)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_controller(face, keyChain)
{
}

void
Procedure::initialize(const Options& options)
{
  BOOST_ASSERT(m_stages.empty());
  this->makeStages(options);
  BOOST_ASSERT(!m_stages.empty());

  for (size_t i = 0; i < m_stages.size(); ++i) {
    m_stages[i]->onSuccess.connect(bind(&Procedure::connect, this, _1));
    if (i + 1 < m_stages.size()) {
      m_stages[i]->onFailure.connect([=] (const std::string&) { m_stages[i + 1]->start(); });
    }
    else {
      m_stages[i]->onFailure.connect([=] (const std::string&) { this->onComplete(false); });
    }
  }
}

void
Procedure::makeStages(const Options& options)
{
  m_stages.push_back(make_unique<MulticastDiscovery>(m_face, m_controller));
  m_stages.push_back(make_unique<GuessFromSearchDomains>());
  m_stages.push_back(make_unique<NdnFchDiscovery>(options.ndnFchUrl));
  m_stages.push_back(make_unique<GuessFromIdentityName>(m_keyChain));
}

void
Procedure::runOnce()
{
  BOOST_ASSERT(!m_stages.empty());
  m_stages.front()->start();
}

void
Procedure::connect(const FaceUri& hubFaceUri)
{
  hubFaceUri.canonize(
    [this] (const FaceUri& canonicalUri) {
      m_controller.start<nfd::FaceCreateCommand>(
        ControlParameters().setUri(canonicalUri.toString()),
        [this] (const ControlParameters& params) {
          std::cerr << "Connected to HUB " << params.getUri() << std::endl;
          this->registerPrefixes(params.getFaceId());
        },
        [this, canonicalUri] (const ControlResponse& resp) {
          if (resp.getCode() == 409) {
            ControlParameters params(resp.getBody());
            std::cerr << "Already connected to HUB " << params.getUri() << std::endl;
            this->registerPrefixes(params.getFaceId());
          }
          else {
            std::cerr << "Failed to connect to HUB " << canonicalUri << ": "
                      << resp.getText() << " (" << resp.getCode() << ")" << std::endl;
            this->onComplete(false);
          }
        });
    },
    [this] (const std::string& reason) {
      std::cerr << "Failed to canonize HUB FaceUri: " << reason << std::endl;
      this->onComplete(false);
    },
    m_face.getIoService(), FACEURI_CANONIZE_TIMEOUT);
}

void
Procedure::registerPrefixes(uint64_t hubFaceId, size_t index)
{
  if (index >= HUB_PREFIXES.size()) {
    this->onComplete(true);
    return;
  }

  m_controller.start<nfd::RibRegisterCommand>(
    ControlParameters()
      .setName(HUB_PREFIXES[index])
      .setFaceId(hubFaceId)
      .setOrigin(HUB_ROUTE_ORIGIN)
      .setCost(HUB_ROUTE_COST),
    [=] (const ControlParameters&) {
      std::cerr << "Registered prefix " << HUB_PREFIXES[index] << std::endl;
      this->registerPrefixes(hubFaceId, index + 1);
    },
    [=] (const ControlResponse& resp) {
      std::cerr << "Failed to register " << HUB_PREFIXES[index] << ": "
                << resp.getText() << " (" << resp.getCode() << ")" << std::endl;
      this->onComplete(false);
    });
}

} // namespace autoconfig
} // namespace tools
} // namespace ndn
