/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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

#ifndef NFD_DAEMON_TABLE_FIB_HPP
#define NFD_DAEMON_TABLE_FIB_HPP

#include "fib-entry.hpp"
#include "name-tree.hpp"

namespace nfd {

namespace measurements {
class Entry;
}
namespace pit {
class Entry;
}

/** \class Fib
 *  \brief represents the FIB
 */
class Fib : noncopyable
{
public:
  explicit
  Fib(NameTree& nameTree);

  ~Fib();

  size_t
  size() const;

public: // lookup
  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const Name& prefix) const;

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const pit::Entry& pitEntry) const;

  /// performs a longest prefix match
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const;

  shared_ptr<fib::Entry>
  findExactMatch(const Name& prefix) const;

public: // mutation
  /** \brief inserts a FIB entry for prefix
   *  If an entry for exact same prefix exists, that entry is returned.
   *  \return{ the entry, and true for new entry, false for existing entry }
   */
  std::pair<shared_ptr<fib::Entry>, bool>
  insert(const Name& prefix);

  void
  erase(const Name& prefix);

  void
  erase(const fib::Entry& entry);

  /** \brief removes the NextHop record for face in all entrites
   *
   *  This is usually invoked when face goes away.
   *  Removing the last NextHop in a FIB entry will erase the FIB entry.
   *
   *  \todo change parameter type to Face&
   */
  void
  removeNextHopFromAllEntries(shared_ptr<Face> face);

public: // enumeration
  class const_iterator;

  /** \brief returns an iterator pointing to the first FIB entry
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated if FIB or another NameTree-based
   *        table is modified
   */
  const_iterator
  begin() const;

  /** \brief returns an iterator referring to the past-the-end FIB entry
   *  \note The returned iterator may get invalidated if FIB or another NameTree-based
   *        table is modified
   */
  const_iterator
  end() const;

  class const_iterator : public std::iterator<std::forward_iterator_tag, const fib::Entry>
  {
  public:
    const_iterator() = default;

    explicit
    const_iterator(const NameTree::const_iterator& it);

    ~const_iterator();

    const fib::Entry&
    operator*() const;

    shared_ptr<fib::Entry>
    operator->() const;

    const_iterator&
    operator++();

    const_iterator
    operator++(int);

    bool
    operator==(const const_iterator& other) const;

    bool
    operator!=(const const_iterator& other) const;

  private:
    NameTree::const_iterator m_nameTreeIterator;
  };

private:
  shared_ptr<fib::Entry>
  findLongestPrefixMatch(shared_ptr<name_tree::Entry> nameTreeEntry) const;

  void
  erase(shared_ptr<name_tree::Entry> nameTreeEntry);

private:
  NameTree& m_nameTree;
  size_t m_nItems;

  /** \brief The empty FIB entry.
   *
   *  This entry has no nexthops.
   *  It is returned by findLongestPrefixMatch if nothing is matched.
   *  Returning empty entry instead of nullptr makes forwarding and strategy implementation easier.
   */
  static const shared_ptr<fib::Entry> s_emptyEntry;
};

inline size_t
Fib::size() const
{
  return m_nItems;
}

inline Fib::const_iterator
Fib::end() const
{
  return const_iterator(m_nameTree.end());
}

inline
Fib::const_iterator::const_iterator(const NameTree::const_iterator& it)
  : m_nameTreeIterator(it)
{
}

inline
Fib::const_iterator::~const_iterator()
{
}

inline
Fib::const_iterator
Fib::const_iterator::operator++(int)
{
  Fib::const_iterator temp(*this);
  ++(*this);
  return temp;
}

inline Fib::const_iterator&
Fib::const_iterator::operator++()
{
  ++m_nameTreeIterator;
  return *this;
}

inline const fib::Entry&
Fib::const_iterator::operator*() const
{
  return *this->operator->();
}

inline shared_ptr<fib::Entry>
Fib::const_iterator::operator->() const
{
  return m_nameTreeIterator->getFibEntry();
}

inline bool
Fib::const_iterator::operator==(const Fib::const_iterator& other) const
{
  return m_nameTreeIterator == other.m_nameTreeIterator;
}

inline bool
Fib::const_iterator::operator!=(const Fib::const_iterator& other) const
{
  return m_nameTreeIterator != other.m_nameTreeIterator;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_HPP
