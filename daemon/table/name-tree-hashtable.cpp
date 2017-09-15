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

#include "name-tree-hashtable.hpp"
#include "core/logger.hpp"
#include "core/city-hash.hpp"

namespace nfd {
namespace name_tree {

NFD_LOG_INIT("NameTreeHashtable");

class Hash32
{
public:
  static HashValue
  compute(const void* buffer, size_t length)
  {
    return static_cast<HashValue>(CityHash32(reinterpret_cast<const char*>(buffer), length));
  }
};

class Hash64
{
public:
  static HashValue
  compute(const void* buffer, size_t length)
  {
    return static_cast<HashValue>(CityHash64(reinterpret_cast<const char*>(buffer), length));
  }
};

/** \brief a type with compute static method to compute hash value from a raw buffer
 */
using HashFunc = std::conditional<(sizeof(HashValue) > 4), Hash64, Hash32>::type;

HashValue
computeHash(const Name& name, size_t prefixLen)
{
  name.wireEncode(); // ensure wire buffer exists

  HashValue h = 0;
  for (size_t i = 0, last = std::min(prefixLen, name.size()); i < last; ++i) {
    const name::Component& comp = name[i];
    h ^= HashFunc::compute(comp.wire(), comp.size());
  }
  return h;
}

HashSequence
computeHashes(const Name& name, size_t prefixLen)
{
  name.wireEncode(); // ensure wire buffer exists

  size_t last = std::min(prefixLen, name.size());
  HashSequence seq;
  seq.reserve(last + 1);

  HashValue h = 0;
  seq.push_back(h);

  for (size_t i = 0; i < last; ++i) {
    const name::Component& comp = name[i];
    h ^= HashFunc::compute(comp.wire(), comp.size());
    seq.push_back(h);
  }
  return seq;
}

Node::Node(HashValue h, const Name& name)
  : hash(h)
  , prev(nullptr)
  , next(nullptr)
  , entry(name, this)
{
}

Node::~Node()
{
  BOOST_ASSERT(prev == nullptr);
  BOOST_ASSERT(next == nullptr);
}

Node*
getNode(const Entry& entry)
{
  return entry.m_node;
}

HashtableOptions::HashtableOptions(size_t size)
  : initialSize(size)
  , minSize(size)
{
}

Hashtable::Hashtable(const Options& options)
  : m_options(options)
  , m_size(0)
{
  BOOST_ASSERT(m_options.minSize > 0);
  BOOST_ASSERT(m_options.initialSize >= m_options.minSize);
  BOOST_ASSERT(m_options.expandLoadFactor > 0.0);
  BOOST_ASSERT(m_options.expandLoadFactor <= 1.0);
  BOOST_ASSERT(m_options.expandFactor > 1.0);
  BOOST_ASSERT(m_options.shrinkLoadFactor >= 0.0);
  BOOST_ASSERT(m_options.shrinkLoadFactor < 1.0);
  BOOST_ASSERT(m_options.shrinkFactor > 0.0);
  BOOST_ASSERT(m_options.shrinkFactor < 1.0);

  m_buckets.resize(options.initialSize);
  this->computeThresholds();
}

Hashtable::~Hashtable()
{
  for (size_t i = 0; i < m_buckets.size(); ++i) {
    foreachNode(m_buckets[i], [] (Node* node) {
      node->prev = node->next = nullptr;
      delete node;
    });
  }
}

void
Hashtable::attach(size_t bucket, Node* node)
{
  node->prev = nullptr;
  node->next = m_buckets[bucket];

  if (node->next != nullptr) {
    BOOST_ASSERT(node->next->prev == nullptr);
    node->next->prev = node;
  }

  m_buckets[bucket] = node;
}

void
Hashtable::detach(size_t bucket, Node* node)
{
  if (node->prev != nullptr) {
    BOOST_ASSERT(node->prev->next == node);
    node->prev->next = node->next;
  }
  else {
    BOOST_ASSERT(m_buckets[bucket] == node);
    m_buckets[bucket] = node->next;
  }

  if (node->next != nullptr) {
    BOOST_ASSERT(node->next->prev == node);
    node->next->prev = node->prev;
  }

  node->prev = node->next = nullptr;
}

std::pair<const Node*, bool>
Hashtable::findOrInsert(const Name& name, size_t prefixLen, HashValue h, bool allowInsert)
{
  size_t bucket = this->computeBucketIndex(h);

  for (const Node* node = m_buckets[bucket]; node != nullptr; node = node->next) {
    if (node->hash == h && name.compare(0, prefixLen, node->entry.getName()) == 0) {
      NFD_LOG_TRACE("found " << name.getPrefix(prefixLen) << " hash=" << h << " bucket=" << bucket);
      return {node, false};
    }
  }

  if (!allowInsert) {
    NFD_LOG_TRACE("not-found " << name.getPrefix(prefixLen) << " hash=" << h << " bucket=" << bucket);
    return {nullptr, false};
  }

  Node* node = new Node(h, name.getPrefix(prefixLen));
  this->attach(bucket, node);
  NFD_LOG_TRACE("insert " << node->entry.getName() << " hash=" << h << " bucket=" << bucket);
  ++m_size;

  if (m_size > m_expandThreshold) {
    this->resize(static_cast<size_t>(m_options.expandFactor * this->getNBuckets()));
  }

  return {node, true};
}

const Node*
Hashtable::find(const Name& name, size_t prefixLen) const
{
  HashValue h = computeHash(name, prefixLen);
  return const_cast<Hashtable*>(this)->findOrInsert(name, prefixLen, h, false).first;
}

const Node*
Hashtable::find(const Name& name, size_t prefixLen, const HashSequence& hashes) const
{
  BOOST_ASSERT(hashes.at(prefixLen) == computeHash(name, prefixLen));
  return const_cast<Hashtable*>(this)->findOrInsert(name, prefixLen, hashes[prefixLen], false).first;
}

std::pair<const Node*, bool>
Hashtable::insert(const Name& name, size_t prefixLen, const HashSequence& hashes)
{
  BOOST_ASSERT(hashes.at(prefixLen) == computeHash(name, prefixLen));
  return this->findOrInsert(name, prefixLen, hashes[prefixLen], true);
}

void
Hashtable::erase(Node* node)
{
  BOOST_ASSERT(node != nullptr);
  BOOST_ASSERT(node->entry.getParent() == nullptr);

  size_t bucket = this->computeBucketIndex(node->hash);
  NFD_LOG_TRACE("erase " << node->entry.getName() << " hash=" << node->hash << " bucket=" << bucket);

  this->detach(bucket, node);
  delete node;
  --m_size;

  if (m_size < m_shrinkThreshold) {
    size_t newNBuckets = std::max(m_options.minSize,
      static_cast<size_t>(m_options.shrinkFactor * this->getNBuckets()));
    this->resize(newNBuckets);
  }
}

void
Hashtable::computeThresholds()
{
  m_expandThreshold = static_cast<size_t>(m_options.expandLoadFactor * this->getNBuckets());
  m_shrinkThreshold = static_cast<size_t>(m_options.shrinkLoadFactor * this->getNBuckets());
  NFD_LOG_TRACE("thresholds expand=" << m_expandThreshold << " shrink=" << m_shrinkThreshold);
}

void
Hashtable::resize(size_t newNBuckets)
{
  if (this->getNBuckets() == newNBuckets) {
    return;
  }
  NFD_LOG_DEBUG("resize from=" << this->getNBuckets() << " to=" << newNBuckets);

  std::vector<Node*> oldBuckets;
  oldBuckets.swap(m_buckets);
  m_buckets.resize(newNBuckets);

  for (Node* head : oldBuckets) {
    foreachNode(head, [this] (Node* node) {
      size_t bucket = this->computeBucketIndex(node->hash);
      this->attach(bucket, node);
    });
  }

  this->computeThresholds();
}

} // namespace name_tree
} // namespace nfd
