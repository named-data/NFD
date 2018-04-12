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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP

#include "name-tree-hashtable.hpp"

namespace nfd {
namespace name_tree {

class NameTree;

/** \brief a predicate to accept or reject an Entry in find operations
 */
using EntrySelector = std::function<bool(const Entry&)>;

/** \brief an EntrySelector that accepts every Entry
 */
struct AnyEntry
{
  bool
  operator()(const Entry&) const
  {
    return true;
  }
};

/** \brief a predicate to accept or reject an Entry and its children
 *  \return `.first` indicates whether entry should be accepted;
 *          `.second` indicates whether entry's children should be visited
 */
using EntrySubTreeSelector = std::function<std::pair<bool, bool>(const Entry&)>;

/** \brief an EntrySubTreeSelector that accepts every Entry and its children
 */
struct AnyEntrySubTree
{
  std::pair<bool, bool>
  operator()(const Entry&) const
  {
    return {true, true};
  }
};

class EnumerationImpl;

/** \brief NameTree iterator
 */
class Iterator : public std::iterator<std::forward_iterator_tag, const Entry>
{
public:
  Iterator();

  Iterator(shared_ptr<EnumerationImpl> impl, const Entry* ref);

  const Entry&
  operator*() const
  {
    BOOST_ASSERT(m_impl != nullptr);
    return *m_entry;
  }

  const Entry*
  operator->() const
  {
    BOOST_ASSERT(m_impl != nullptr);
    return m_entry;
  }

  Iterator&
  operator++();

  Iterator
  operator++(int);

  bool
  operator==(const Iterator& other) const;

  bool
  operator!=(const Iterator& other) const
  {
    return !this->operator==(other);
  }

private:
  /** \brief enumeration implementation; nullptr for end iterator
   */
  shared_ptr<EnumerationImpl> m_impl;

  /** \brief current entry; nullptr for uninitialized iterator
   */
  const Entry* m_entry;

  /** \brief reference entry used by enumeration implementation
   */
  const Entry* m_ref;

  /** \brief state used by enumeration implementation
   */
  int m_state;

  friend std::ostream& operator<<(std::ostream&, const Iterator&);
  friend class FullEnumerationImpl;
  friend class PartialEnumerationImpl;
  friend class PrefixMatchImpl;
};

std::ostream&
operator<<(std::ostream& os, const Iterator& i);

/** \brief enumeration operation implementation
 */
class EnumerationImpl
{
public:
  explicit
  EnumerationImpl(const NameTree& nt);

  virtual
  ~EnumerationImpl() = default;

  virtual void
  advance(Iterator& i) = 0;

protected:
  const NameTree& nt;
  const Hashtable& ht;
};

/** \brief full enumeration implementation
 */
class FullEnumerationImpl : public EnumerationImpl
{
public:
  FullEnumerationImpl(const NameTree& nt, const EntrySelector& pred);

  void
  advance(Iterator& i) override;

private:
  EntrySelector m_pred;
};

/** \brief partial enumeration implementation
 *
 *  Iterator::m_ref should be initialized to subtree root.
 *  Iterator::m_state LSB indicates whether to visit children of m_entry.
 */
class PartialEnumerationImpl : public EnumerationImpl
{
public:
  PartialEnumerationImpl(const NameTree& nt, const EntrySubTreeSelector& pred);

  void
  advance(Iterator& i) override;

private:
  EntrySubTreeSelector m_pred;
};

/** \brief partial enumeration implementation
 *
 *  Iterator::m_ref should be initialized to longest prefix matched entry.
 */
class PrefixMatchImpl : public EnumerationImpl
{
public:
  PrefixMatchImpl(const NameTree& nt, const EntrySelector& pred);

private:
  void
  advance(Iterator& i) override;

private:
  EntrySelector m_pred;
};

/** \brief a Forward Range of name tree entries
 *
 *  This type has .begin() and .end() methods which return Iterator.
 *  This type is usable with range-based for.
 */
using Range = boost::iterator_range<Iterator>;

} // namespace name_tree
} // namespace nfd

#endif // NFD_DAEMON_TABLE_NAME_TREE_ITERATOR_HPP
