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

#ifndef NFD_DAEMON_TABLE_FIB_HPP
#define NFD_DAEMON_TABLE_FIB_HPP

#include "fib-entry.hpp"
#include "name-tree.hpp"

#include <boost/range/adaptor/transformed.hpp>

namespace nfd {

namespace measurements {
class Entry;
} // namespace measurements
namespace pit {
class Entry;
} // namespace pit

namespace fib {

/** \brief Represents the Forwarding Information Base (FIB)
 */
class Fib : noncopyable
{
public:
  explicit
  Fib(NameTree& nameTree);

  size_t
  size() const
  {
    return m_nItems;
  }

public: // lookup
  /** \brief Performs a longest prefix match
   */
  const Entry&
  findLongestPrefixMatch(const Name& prefix) const;

  /** \brief Performs a longest prefix match
   *
   *  This is equivalent to `findLongestPrefixMatch(pitEntry.getName())`
   */
  const Entry&
  findLongestPrefixMatch(const pit::Entry& pitEntry) const;

  /** \brief Performs a longest prefix match
   *
   *  This is equivalent to `findLongestPrefixMatch(measurementsEntry.getName())`
   */
  const Entry&
  findLongestPrefixMatch(const measurements::Entry& measurementsEntry) const;

  /** \brief Performs an exact match lookup
   */
  Entry*
  findExactMatch(const Name& prefix);

public: // mutation
  /** \brief Maximum number of components in a FIB entry prefix.
   */
  static constexpr size_t
  getMaxDepth()
  {
    return NameTree::getMaxDepth();
  }

  /** \brief Find or insert a FIB entry
   *  \param prefix FIB entry name; it must not have more than \c getMaxDepth() components.
   *  \return the entry, and true for new entry or false for existing entry
   */
  std::pair<Entry*, bool>
  insert(const Name& prefix);

  void
  erase(const Name& prefix);

  void
  erase(const Entry& entry);

  /** \brief Add a NextHop record
   *
   *  If a NextHop record for \p face already exists in \p entry, its cost is set to \p cost.
   */
  void
  addOrUpdateNextHop(Entry& entry, Face& face, uint64_t cost);

  enum class RemoveNextHopResult {
    NO_SUCH_NEXTHOP, ///< the nexthop is not found
    NEXTHOP_REMOVED, ///< the nexthop is removed and the fib entry stays
    FIB_ENTRY_REMOVED ///< the nexthop is removed and the fib entry is removed
  };

  /** \brief Remove the NextHop record for \p face from \p entry
   */
  RemoveNextHopResult
  removeNextHop(Entry& entry, const Face& face);

public: // enumeration
  typedef boost::transformed_range<name_tree::GetTableEntry<Entry>, const name_tree::Range> Range;
  typedef boost::range_iterator<Range>::type const_iterator;

  /** \return an iterator to the beginning
   *  \note The iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during iteration.
   */
  const_iterator
  begin() const
  {
    return this->getRange().begin();
  }

  /** \return an iterator to the end
   *  \sa begin()
   */
  const_iterator
  end() const
  {
    return this->getRange().end();
  }

public: // signal
  /** \brief signals on Fib entry nexthop creation
   */
  signal::Signal<Fib, Name, NextHop> afterNewNextHop;

private:
  /** \tparam K a parameter acceptable to NameTree::findLongestPrefixMatch
   */
  template<typename K>
  const Entry&
  findLongestPrefixMatchImpl(const K& key) const;

  void
  erase(name_tree::Entry* nte, bool canDeleteNte = true);

  Range
  getRange() const;

private:
  NameTree& m_nameTree;
  size_t m_nItems = 0;

  /** \brief The empty FIB entry.
   *
   *  This entry has no nexthops.
   *  It is returned by findLongestPrefixMatch if nothing is matched.
   */
  static const unique_ptr<Entry> s_emptyEntry;
};

} // namespace fib

using fib::Fib;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_FIB_HPP
