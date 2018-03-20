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

#ifndef NFD_DAEMON_TABLE_MEASUREMENTS_HPP
#define NFD_DAEMON_TABLE_MEASUREMENTS_HPP

#include "measurements-entry.hpp"
#include "name-tree.hpp"

namespace nfd {

namespace fib {
class Entry;
} // namespace fib

namespace pit {
class Entry;
} // namespace pit

namespace measurements {

/** \brief a predicate that accepts or rejects an entry
 */
typedef std::function<bool(const Entry&)> EntryPredicate;

/** \brief an \c EntryPredicate that accepts any entry
 */
class AnyEntry
{
public:
  bool
  operator()(const Entry& entry) const
  {
    return true;
  }
};

/** \brief an \c EntryPredicate that accepts an entry if it has StrategyInfo of type T
 */
template<typename T>
class EntryWithStrategyInfo
{
public:
  bool
  operator()(const Entry& entry) const
  {
    return entry.getStrategyInfo<T>() != nullptr;
  }
};

/** \brief the Measurements table
 *
 *  The Measurements table is a data structure for forwarding strategies to store per name prefix
 *  measurements. A strategy can access this table via \c Strategy::getMeasurements(), and then
 *  place any object that derive from \c StrategyInfo type onto Measurements entries.
 */
class Measurements : noncopyable
{
public:
  explicit
  Measurements(NameTree& nameTree);

  /** \brief maximum depth of a Measurements entry
   */
  static constexpr size_t
  getMaxDepth()
  {
    return NameTree::getMaxDepth();
  }

  /** \brief find or insert an entry by name
   *
   *  An entry name can have at most \c getMaxDepth() components. If \p name exceeds this limit,
   *  it is truncated to the first \c getMaxDepth() components.
   */
  Entry&
  get(const Name& name);

  /** \brief equivalent to `get(fibEntry.getPrefix())`
   */
  Entry&
  get(const fib::Entry& fibEntry);

  /** \brief equivalent to
   *         `get(pitEntry.getName(), std::min(pitEntry.getName().size(), getMaxDepth()))`
   */
  Entry&
  get(const pit::Entry& pitEntry);

  /** \brief find or insert a parent entry
   *  \retval nullptr child is the root entry
   *  \return get(child.getName().getPrefix(-1))
   */
  Entry*
  getParent(const Entry& child);

  /** \brief perform a longest prefix match for \p name
   */
  Entry*
  findLongestPrefixMatch(const Name& name,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief perform a longest prefix match for `pitEntry.getName()`
   */
  Entry*
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const EntryPredicate& pred = AnyEntry()) const;

  /** \brief perform an exact match
   */
  Entry*
  findExactMatch(const Name& name) const;

  static time::nanoseconds
  getInitialLifetime();

  /** \brief extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(Entry& entry, const time::nanoseconds& lifetime);

  size_t
  size() const;

private:
  void
  cleanup(Entry& entry);

  Entry&
  get(name_tree::Entry& nte);

  /** \tparam K a parameter acceptable to \c NameTree::findLongestPrefixMatch
   */
  template<typename K>
  Entry*
  findLongestPrefixMatchImpl(const K& key, const EntryPredicate& pred) const;

private:
  NameTree& m_nameTree;
  size_t m_nItems;
};

inline time::nanoseconds
Measurements::getInitialLifetime()
{
  return time::seconds(4);
}

inline size_t
Measurements::size() const
{
  return m_nItems;
}

} // namespace measurements

using measurements::Measurements;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_HPP
