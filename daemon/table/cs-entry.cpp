/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 *
 * Author: Ilya Moiseenko <iliamo@ucla.edu>
 */

#include "cs-entry.hpp"
#include "core/logger.hpp"

namespace nfd {
namespace cs {

NFD_LOG_INIT("CsEntry");

Entry::Entry(const Data& data, bool isUnsolicited)
  : m_dataPacket(data.shared_from_this())
  , m_isUnsolicited(isUnsolicited)
  , m_wasRefreshedByDuplicate(false)
  , m_nameWithDigest(data.getName())
{
  updateStaleTime();
  m_nameWithDigest.append(ndn::name::Component(getDigest()));
}


Entry::~Entry()
{
}

const Name&
Entry::getName() const
{
  return m_nameWithDigest;
}

const Data&
Entry::getData() const
{
  return *m_dataPacket;
}

void
Entry::setData(const Data& data)
{
  /// \todo This method may not be necessary (if it is real duplicate,
  ///       there is no reason to recalculate the same digest

  m_dataPacket = data.shared_from_this();
  m_digest.reset();
  m_wasRefreshedByDuplicate = true;

  updateStaleTime();

  m_nameWithDigest = data.getName();
  m_nameWithDigest.append(ndn::name::Component(getDigest()));
}

void
Entry::setData(const Data& data, const ndn::ConstBufferPtr& digest)
{
  /// \todo This method may not be necessary (if it is real duplicate,
  ///       there is no reason to recalculate the same digest

  m_dataPacket = data.shared_from_this();
  m_digest = digest;
  m_wasRefreshedByDuplicate = true;

  updateStaleTime();

  m_nameWithDigest = data.getName();
  m_nameWithDigest.append(ndn::name::Component(getDigest()));
}

bool
Entry::isUnsolicited() const
{
  return m_isUnsolicited;
}

const time::steady_clock::TimePoint&
Entry::getStaleTime() const
{
  return m_staleAt;
}

void
Entry::updateStaleTime()
{
  m_staleAt = time::steady_clock::now() + m_dataPacket->getFreshnessPeriod();
}

bool
Entry::wasRefreshedByDuplicate() const
{
  return m_wasRefreshedByDuplicate;
}

const ndn::ConstBufferPtr&
Entry::getDigest() const
{
  if (!static_cast<bool>(m_digest))
    {
      const Block& block = m_dataPacket->wireEncode();
      m_digest = ndn::crypto::sha256(block.wire(), block.size());
    }

  return m_digest;
}

void
Entry::setIterator(int layer, const Entry::LayerIterators::mapped_type& layerIterator)
{
  m_layerIterators[layer] = layerIterator;
}

void
Entry::removeIterator(int layer)
{
  m_layerIterators.erase(layer);
}

const Entry::LayerIterators&
Entry::getIterators() const
{
  return m_layerIterators;
}

void
Entry::printIterators() const
{
  for (LayerIterators::const_iterator it = m_layerIterators.begin();
       it != m_layerIterators.end();
       ++it)
    {
      NFD_LOG_DEBUG("[" << it->first << "]" << " " << &(*it->second));
    }
}

} // namespace cs
} // namespace nfd
