/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  Regents of the University of California,
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

#include "protocol-factory.hpp"

#include <boost/range/adaptor/map.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace nfd {
namespace face {

ProtocolFactory::Registry&
ProtocolFactory::getRegistry()
{
  static Registry registry;
  return registry;
}

unique_ptr<ProtocolFactory>
ProtocolFactory::create(const std::string& id, const CtorParams& params)
{
  Registry& registry = getRegistry();
  auto found = registry.find(id);
  if (found == registry.end()) {
    return nullptr;
  }

  return found->second(params);
}

std::set<std::string>
ProtocolFactory::listRegistered()
{
  std::set<std::string> factoryIds;
  boost::copy(getRegistry() | boost::adaptors::map_keys,
              std::inserter(factoryIds, factoryIds.end()));
  return factoryIds;
}

ProtocolFactory::ProtocolFactory(const CtorParams& params)
  : addFace(params.addFace)
  , netmon(params.netmon)
{
  BOOST_ASSERT(addFace != nullptr);
  BOOST_ASSERT(netmon != nullptr);
}

ProtocolFactory::~ProtocolFactory() = default;

void
ProtocolFactory::processConfig(OptionalConfigSection configSection,
                               FaceSystem::ConfigContext& context)
{
  doProcessConfig(configSection, context);
}

void
ProtocolFactory::doProcessConfig(OptionalConfigSection,
                                 FaceSystem::ConfigContext&)
{
}

void
ProtocolFactory::createFace(const CreateFaceRequest& req,
                            const FaceCreatedCallback& onCreated,
                            const FaceCreationFailedCallback& onFailure)
{
  BOOST_ASSERT(!FaceUri::canCanonize(req.remoteUri.getScheme()) ||
               req.remoteUri.isCanonical());
  BOOST_ASSERT(!req.localUri || !FaceUri::canCanonize(req.localUri->getScheme()) ||
               req.localUri->isCanonical());
  doCreateFace(req, onCreated, onFailure);
}

void
ProtocolFactory::doCreateFace(const CreateFaceRequest&,
                              const FaceCreatedCallback&,
                              const FaceCreationFailedCallback& onFailure)
{
  onFailure(406, "Unsupported protocol");
}

shared_ptr<Face>
ProtocolFactory::createNetdevBoundFace(const FaceUri& remote,
                                       const shared_ptr<const ndn::net::NetworkInterface>& netif)
{
  BOOST_ASSERT(remote.isCanonical());
  return doCreateNetdevBoundFace(remote, netif);
}

shared_ptr<Face>
ProtocolFactory::doCreateNetdevBoundFace(const FaceUri&,
                                         const shared_ptr<const ndn::net::NetworkInterface>&)
{
  NDN_THROW(Error("This protocol factory does not support netdev-bound faces"));
}

std::vector<shared_ptr<const Channel>>
ProtocolFactory::getChannels() const
{
  return doGetChannels();
}

std::vector<shared_ptr<const Channel>>
ProtocolFactory::doGetChannels() const
{
  return {};
}

} // namespace face
} // namespace nfd
