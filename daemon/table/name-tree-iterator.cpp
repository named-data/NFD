/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2018,  Regents of the University of California,
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

#include "name-tree-iterator.hpp"
#include "name-tree.hpp"
#include "core/logger.hpp"

#include <boost/range/concepts.hpp>
#include <ndn-cxx/util/concepts.hpp>

namespace nfd {
namespace name_tree {

NDN_CXX_ASSERT_FORWARD_ITERATOR(Iterator);
BOOST_CONCEPT_ASSERT((boost::ForwardRangeConcept<Range>));

NFD_LOG_INIT("NameTreeIterator");

Iterator::Iterator()
  : m_entry(nullptr)
  , m_ref(nullptr)
  , m_state(0)
{
}

Iterator::Iterator(shared_ptr<EnumerationImpl> impl, const Entry* ref)
  : m_impl(impl)
  , m_entry(nullptr)
  , m_ref(ref)
  , m_state(0)
{
  m_impl->advance(*this);
  NFD_LOG_TRACE("initialized " << *this);
}

Iterator&
Iterator::operator++()
{
  BOOST_ASSERT(m_impl != nullptr);
  m_impl->advance(*this);
  NFD_LOG_TRACE("advanced " << *this);
  return *this;
}

Iterator
Iterator::operator++(int)
{
  Iterator copy = *this;
  this->operator++();
  return copy;
}

bool
Iterator::operator==(const Iterator& other) const
{
  return m_entry == other.m_entry;
}

std::ostream&
operator<<(std::ostream& os, const Iterator& i)
{
  if (i.m_impl == nullptr) {
    return os << "end";
  }
  if (i.m_entry == nullptr) {
    return os << "uninitialized";
  }

  os << "entry=" << i.m_entry->getName();
  if (i.m_ref == nullptr) {
    os << " ref=null";
  }
  else {
    os << " ref=" << i.m_ref->getName();
  }
  os << " state=" << i.m_state;
  return os;
}

EnumerationImpl::EnumerationImpl(const NameTree& nt)
  : nt(nt)
  , ht(nt.m_ht)
{
}

FullEnumerationImpl::FullEnumerationImpl(const NameTree& nt, const EntrySelector& pred)
  : EnumerationImpl(nt)
  , m_pred(pred)
{
}

void
FullEnumerationImpl::advance(Iterator& i)
{
  // find first entry
  if (i.m_entry == nullptr) {
    for (size_t bucket = 0; bucket < ht.getNBuckets(); ++bucket) {
      const Node* node = ht.getBucket(bucket);
      if (node != nullptr) {
        i.m_entry = &node->entry;
        break;
      }
    }
    if (i.m_entry == nullptr) { // empty enumerable
      i = Iterator();
      return;
    }
    if (m_pred(*i.m_entry)) { // visit first entry
      return;
    }
  }

  // process entries in same bucket
  for (const Node* node = getNode(*i.m_entry)->next; node != nullptr; node = node->next) {
    if (m_pred(node->entry)) {
      i.m_entry = &node->entry;
      return;
    }
  }

  // process other buckets
  size_t currentBucket = ht.computeBucketIndex(getNode(*i.m_entry)->hash);
  for (size_t bucket = currentBucket + 1; bucket < ht.getNBuckets(); ++bucket) {
    for (const Node* node = ht.getBucket(bucket); node != nullptr; node = node->next) {
      if (m_pred(node->entry)) {
        i.m_entry = &node->entry;
        return;
      }
    }
  }

  // reach the end
  i = Iterator();
}

PartialEnumerationImpl::PartialEnumerationImpl(const NameTree& nt, const EntrySubTreeSelector& pred)
  : EnumerationImpl(nt)
  , m_pred(pred)
{
}

void
PartialEnumerationImpl::advance(Iterator& i)
{
  bool wantSelf = false;
  bool wantChildren = false;

  // initialize: start from root
  if (i.m_entry == nullptr) {
    if (i.m_ref == nullptr) { // root does not exist
      i = Iterator();
      return;
    }

    i.m_entry = i.m_ref;
    std::tie(wantSelf, wantChildren) = m_pred(*i.m_entry);
    if (wantSelf) { // visit root
      i.m_state = wantChildren;
      return;
    }
  }
  else {
    wantChildren = static_cast<bool>(i.m_state);
  }
  BOOST_ASSERT(i.m_ref != nullptr);

  // pre-order traversal
  while (i.m_entry != i.m_ref || (wantChildren && i.m_entry->hasChildren())) {
    if (wantChildren && i.m_entry->hasChildren()) { // process children of m_entry
      i.m_entry = i.m_entry->getChildren().front();
      std::tie(wantSelf, wantChildren) = m_pred(*i.m_entry);
      if (wantSelf) { // visit first child
        i.m_state = wantChildren;
        return;
      }
      // first child rejected, let while loop process other children (siblings of new m_entry)
    }
    else { // process siblings of m_entry
      const Entry* parent = i.m_entry->getParent();
      const std::vector<Entry*>& siblings = parent->getChildren();
      auto sibling = std::find(siblings.begin(), siblings.end(), i.m_entry);
      BOOST_ASSERT(sibling != siblings.end());
      while (++sibling != siblings.end()) {
        i.m_entry = *sibling;
        std::tie(wantSelf, wantChildren) = m_pred(*i.m_entry);
        if (wantSelf) { // visit sibling
          i.m_state = wantChildren;
          return;
        }
        if (wantChildren) {
          break; // let outer while loop process children of m_entry
        }
        // process next sibling
      }
      if (sibling == siblings.end()) { // no more sibling
        i.m_entry = parent;
        wantChildren = false;
      }
    }
  }

  // reach the end
  i = Iterator();
}

PrefixMatchImpl::PrefixMatchImpl(const NameTree& nt, const EntrySelector& pred)
  : EnumerationImpl(nt)
  , m_pred(pred)
{
}

void
PrefixMatchImpl::advance(Iterator& i)
{
  if (i.m_entry == nullptr) {
    if (i.m_ref == nullptr) { // empty enumerable
      i = Iterator();
      return;
    }

    i.m_entry = i.m_ref;
    if (m_pred(*i.m_entry)) { // visit starting node
      return;
    }
  }

  // traverse up the tree
  while ((i.m_entry = i.m_entry->getParent()) != nullptr) {
    if (m_pred(*i.m_entry)) {
      return;
    }
  }

  // reach the end
  i = Iterator();
}

} // namespace name_tree
} // namespace nfd
