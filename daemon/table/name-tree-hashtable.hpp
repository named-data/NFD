/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_HASHTABLE_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_HASHTABLE_HPP

#include "name-tree-entry.hpp"

namespace nfd::name_tree {

class Entry;

/** \brief A single hash value.
 */
using HashValue = size_t;

/** \brief A sequence of hash values.
 *  \sa computeHashes
 */
using HashSequence = std::vector<HashValue>;

/** \brief Computes hash value of \p name.getPrefix(prefixLen).
 */
HashValue
computeHash(const Name& name, size_t prefixLen = std::numeric_limits<size_t>::max());

/** \brief Computes hash values for each prefix of \p name.getPrefix(prefixLen).
 *  \return a hash sequence, where the i-th hash value equals computeHash(name, i)
 */
HashSequence
computeHashes(const Name& name, size_t prefixLen = std::numeric_limits<size_t>::max());

/** \brief A hashtable node.
 *
 *  Zero or more nodes can be added to a hashtable bucket. They are organized as
 *  a doubly linked list through prev and next pointers.
 */
class Node : noncopyable
{
public:
  /** \post entry.getName() == name
   *  \post getNode(entry) == this
   */
  Node(HashValue h, const Name& name);

  /** \pre prev == nullptr
   *  \pre next == nullptr
   */
  ~Node();

public:
  const HashValue hash;
  Node* prev;
  Node* next;
  mutable Entry entry;
};

/** \return node associated with entry
 *  \note This function is for NameTree internal use.
 */
Node*
getNode(const Entry& entry);

/** \brief Invoke a function for each node in a doubly linked list.
 *  \tparam N either Node or const Node
 *  \tparam F a functor with signature void F(N*)
 *  \note It's safe to delete the node in the function.
 */
template<typename N, typename F>
void
foreachNode(N* head, const F& func)
{
  N* node = head;
  while (node != nullptr) {
    N* next = node->next;
    func(node);
    node = next;
  }
}

/**
 * \brief Provides options for Hashtable.
 */
struct HashtableOptions
{
  /** \brief Constructor.
   *  \post initialSize == size
   *  \post minSize == size
   */
  explicit
  HashtableOptions(size_t size = 16);

  /** \brief Initial number of buckets.
   */
  size_t initialSize;

  /** \brief Minimal number of buckets.
   */
  size_t minSize;

  /** \brief If the hashtable has more than `nBuckets*expandLoadFactor` nodes, it will be expanded.
   */
  float expandLoadFactor = 0.5f;

  /** \brief When the hashtable is expanded, its new size will be `nBuckets*expandFactor`.
   */
  float expandFactor = 2.0f;

  /** \brief If the hashtable has less than `nBuckets*shrinkLoadFactor` nodes, it will be shrunk.
   */
  float shrinkLoadFactor = 0.1f;

  /** \brief When the hashtable is shrunk, its new size will be `max(nBuckets*shrinkFactor, minSize)`.
   */
  float shrinkFactor = 0.5f;
};

/**
 * \brief A hashtable for fast exact name lookup.
 *
 * The Hashtable contains a number of buckets.
 * Each node is placed into a bucket determined by a hash value computed from its name.
 * Hash collision is resolved through a doubly linked list in each bucket.
 * The number of buckets is adjusted according to how many nodes are stored.
 */
class Hashtable
{
public:
  using Options = HashtableOptions;

  explicit
  Hashtable(const Options& options);

  /** \brief Deallocates all nodes.
   */
  ~Hashtable();

  /** \return number of nodes
   */
  size_t
  size() const
  {
    return m_size;
  }

  /** \return number of buckets
   */
  size_t
  getNBuckets() const
  {
    return m_buckets.size();
  }

  /** \return bucket index for hash value h
   */
  size_t
  computeBucketIndex(HashValue h) const
  {
    return h % this->getNBuckets();
  }

  /** \return i-th bucket
   *  \pre bucket < getNBuckets()
   */
  const Node*
  getBucket(size_t bucket) const
  {
    BOOST_ASSERT(bucket < this->getNBuckets());
    return m_buckets[bucket]; // don't use m_bucket.at() for better performance
  }

  /** \brief Find node for name.getPrefix(prefixLen).
   *  \pre name.size() > prefixLen
   */
  const Node*
  find(const Name& name, size_t prefixLen) const;

  /** \brief Find node for name.getPrefix(prefixLen).
   *  \pre name.size() > prefixLen
   *  \pre hashes == computeHashes(name)
   */
  const Node*
  find(const Name& name, size_t prefixLen, const HashSequence& hashes) const;

  /** \brief Find or insert node for name.getPrefix(prefixLen).
   *  \pre name.size() > prefixLen
   *  \pre hashes == computeHashes(name)
   */
  std::pair<const Node*, bool>
  insert(const Name& name, size_t prefixLen, const HashSequence& hashes);

  /** \brief Delete node.
   *  \pre node exists in this hashtable
   */
  void
  erase(Node* node);

private:
  /** \brief Attach node to bucket.
   */
  void
  attach(size_t bucket, Node* node);

  /** \brief Detach node from bucket.
   */
  void
  detach(size_t bucket, Node* node);

  std::pair<const Node*, bool>
  findOrInsert(const Name& name, size_t prefixLen, HashValue h, bool allowInsert);

  void
  computeThresholds();

  void
  resize(size_t newNBuckets);

private:
  std::vector<Node*> m_buckets;
  Options m_options;
  size_t m_size;
  size_t m_expandThreshold;
  size_t m_shrinkThreshold;
};

} // namespace nfd::name_tree

#endif // NFD_DAEMON_TABLE_NAME_TREE_HASHTABLE_HPP
