/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014  Regents of the University of California,
 *                     Arizona Board of Regents,
 *                     Colorado State University,
 *                     University Pierre & Marie Curie, Sorbonne University,
 *                     Washington University in St. Louis,
 *                     Beijing Institute of Technology,
 *                     The University of Memphis
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
 **/

#include "strategy-choice-publisher.hpp"
#include "table/strategy-choice.hpp"

#include <ndn-cxx/management/nfd-strategy-choice.hpp>

namespace nfd {

StrategyChoicePublisher::StrategyChoicePublisher(const StrategyChoice& strategyChoice,
                                                 AppFace& face,
                                                 const Name& prefix,
                                                 ndn::KeyChain& keyChain)
  : SegmentPublisher(face, prefix, keyChain)
  , m_strategyChoice(strategyChoice)
{
}

StrategyChoicePublisher::~StrategyChoicePublisher()
{
}

size_t
StrategyChoicePublisher::generate(ndn::EncodingBuffer& outBuffer)
{
  size_t totalLength = 0;

  for (StrategyChoice::const_iterator i = m_strategyChoice.begin();
       i != m_strategyChoice.end();
       ++i)
    {
      ndn::nfd::StrategyChoice entry;

      entry.setName(i->getPrefix())
           .setStrategy(i->getStrategyName());

      totalLength += entry.wireEncode(outBuffer);
    }

  return totalLength;
}

} // namespace nfd
