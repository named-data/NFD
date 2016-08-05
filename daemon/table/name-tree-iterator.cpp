/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2016,  Regents of the University of California,
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

#include <boost/concept/assert.hpp>
#include <boost/concept_check.hpp>
#include <type_traits>

namespace nfd {
namespace name_tree {

NFD_LOG_INIT("NameTreeIterator");

BOOST_CONCEPT_ASSERT((boost::ForwardIterator<Iterator>));
static_assert(std::is_default_constructible<Iterator>::value,
              "Iterator must be default-constructible");

Iterator::Iterator()
  : m_state(0)
{
}

Iterator::Iterator(shared_ptr<EnumerationImpl> impl, shared_ptr<Entry> ref)
  : m_impl(impl)
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

  os << "entry=" << i.m_entry->getPrefix();
  if (i.m_ref == nullptr) {
    os << " ref=null";
  }
  else {
    os << " ref=" << i.m_ref->getPrefix();
  }
  os << " state=" << i.m_state;
  return os;
}

EnumerationImpl::EnumerationImpl(const NameTree& nt1)
  : nt(nt1)
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
  // find initial entry
  if (i.m_entry == nullptr) {
    for (size_t bucket = 0; bucket < nt.m_nBuckets; ++bucket) {
      Node* node = nt.m_buckets[bucket];
      if (node != nullptr) {
        i.m_entry = node->m_entry;
        break;
      }
    }
    if (i.m_entry == nullptr) { // empty enumeration
      i = Iterator();
      return;
    }
    if (m_pred(*i.m_entry)) { // visit initial entry
      return;
    }
  }

  // process entries in same bucket
  for (Node* node = i.m_entry->m_node->m_next; node != nullptr; node = node->m_next) {
    if (m_pred(*node->m_entry)) {
      i.m_entry = node->m_entry;
      return;
    }
  }

  // process other buckets
  for (size_t bucket = i.m_entry->m_hash % nt.m_nBuckets + 1; bucket < nt.m_nBuckets; ++bucket) {
    for (Node* node = nt.m_buckets[bucket]; node != nullptr; node = node->m_next) {
      if (m_pred(*node->m_entry)) {
        i.m_entry = node->m_entry;
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
    if (i.m_ref == nullptr) { // empty enumeration
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
      shared_ptr<Entry> parent = i.m_entry->getParent();
      const std::vector<shared_ptr<Entry>>& siblings = parent->getChildren();
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
    if (i.m_ref == nullptr) { // empty enumeration
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
