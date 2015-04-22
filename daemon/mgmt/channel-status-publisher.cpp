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

#include "channel-status-publisher.hpp"
#include "face/protocol-factory.hpp"
#include "face/channel.hpp"

#include <ndn-cxx/management/nfd-channel-status.hpp>

namespace nfd {

ChannelStatusPublisher::ChannelStatusPublisher(const FactoryMap& factories,
                                               AppFace& face,
                                               const Name& prefix,
                                               ndn::KeyChain& keyChain)
  : SegmentPublisher(face, prefix, keyChain)
  , m_factories(factories)
{
}

ChannelStatusPublisher::~ChannelStatusPublisher()
{
}

size_t
ChannelStatusPublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;
  std::set<shared_ptr<ProtocolFactory> > seenFactories;

  for (FactoryMap::const_iterator i = m_factories.begin();
       i != m_factories.end(); ++i)
    {
      const shared_ptr<ProtocolFactory>& factory = i->second;

      if (seenFactories.find(factory) != seenFactories.end())
        {
          continue;
        }
      seenFactories.insert(factory);

      std::list<shared_ptr<const Channel> > channels = factory->getChannels();

      for (std::list<shared_ptr<const Channel> >::const_iterator j = channels.begin();
           j != channels.end(); ++j)
        {
          ndn::nfd::ChannelStatus entry;
          entry.setLocalUri((*j)->getUri().toString());

          totalLength += entry.wireEncode(outBuffer);
        }
    }

  return totalLength;
}

} // namespace nfd
