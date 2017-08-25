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

#include "program.hpp"

#include <ndn-cxx/encoding/block-helpers.hpp>
#include <ndn-cxx/encoding/tlv.hpp>
#include <ndn-cxx/encoding/tlv-nfd.hpp>

#include <iostream>

namespace ndn {
namespace tools {
namespace autoconfig_server {

static const Name HUB_DATA_NAME("/localhop/ndn-autoconf/hub");
static const Name ROUTABLE_PREFIXES_DATA_PREFIX("/localhop/nfd");
static const PartialName ROUTABLE_PREFIXES_DATA_SUFFIX("rib/routable-prefixes");

Program::Program(const Options& options, Face& face, KeyChain& keyChain)
  : m_face(face)
  , m_keyChain(keyChain)
  , m_dispatcher(face, keyChain)
{
  this->enableHubData(options.hubFaceUri);
  if (!options.routablePrefixes.empty()) {
    this->enableRoutablePrefixesDataset(options.routablePrefixes);
  }
}

void
Program::enableHubData(const FaceUri& hubFaceUri)
{
  std::string uri = hubFaceUri.toString();

  auto data = make_shared<Data>(Name(HUB_DATA_NAME).appendVersion());
  data->setFreshnessPeriod(time::hours(1));
  data->setContent(makeBinaryBlock(tlv::nfd::Uri,
                                   reinterpret_cast<const uint8_t*>(uri.data()), uri.size()));
  m_keyChain.sign(*data);

  m_face.setInterestFilter(HUB_DATA_NAME,
    [this, data] (const Name&, const Interest& interest) {
      if (interest.matchesData(*data)) {
        m_face.put(*data);
      }
    },
    bind(&Program::handlePrefixRegistrationFailure, this, _1, _2));
}

void
Program::enableRoutablePrefixesDataset(const std::vector<Name>& routablePrefixes)
{
  m_dispatcher.addStatusDataset(ROUTABLE_PREFIXES_DATA_SUFFIX,
    mgmt::makeAcceptAllAuthorization(),
    [=] (const Name& prefix, const Interest& interest, mgmt::StatusDatasetContext& context) {
      for (const Name& routablePrefix : routablePrefixes) {
        context.append(routablePrefix.wireEncode());
      }
      context.end();
    });
  m_dispatcher.addTopPrefix(ROUTABLE_PREFIXES_DATA_PREFIX, false);

  m_face.registerPrefix(Name(ROUTABLE_PREFIXES_DATA_PREFIX).append(ROUTABLE_PREFIXES_DATA_SUFFIX),
    nullptr,
    bind(&Program::handlePrefixRegistrationFailure, this, _1, _2));
}

void
Program::handlePrefixRegistrationFailure(const Name& prefix, const std::string& reason)
{
  std::cerr << "ERROR: cannot register prefix " << prefix
            << " (" << reason << ")" << std::endl;
  m_face.shutdown();
}

} // namespace autoconfig_server
} // namespace tools
} // namespace ndn
