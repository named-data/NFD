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

/** \brief a predicate that accepts or rejects a \p Entry
 */
typedef std::function<bool(const Entry&)> EntryPredicate;

/** \brief an \p EntryPredicate that accepts any \p Entry
 */
class AnyEntry
{
public:
  bool
  operator()(const Entry& entry)
  {
    return true;
  }
};

template<typename T>
class EntryWithStrategyInfo
{
public:
  bool
  operator()(const Entry& entry)
  {
    return entry.getStrategyInfo<T>() != nullptr;
  }
};

} // namespace measurements

/** \brief represents the Measurements table
 */
class Measurements : noncopyable
{
public:
  explicit
  Measurements(NameTree& nametree);

  /** \brief find or insert a Measurements entry for \p name
   */
  shared_ptr<measurements::Entry>
  get(const Name& name);

  /** \brief find or insert a Measurements entry for \p fibEntry.getPrefix()
   */
  shared_ptr<measurements::Entry>
  get(const fib::Entry& fibEntry);

  /** \brief find or insert a Measurements entry for \p pitEntry.getName()
   */
  shared_ptr<measurements::Entry>
  get(const pit::Entry& pitEntry);

  /** \brief find or insert a Measurements entry for child's parent
   *  \retval nullptr if child is the root entry
   */
  shared_ptr<measurements::Entry>
  getParent(const measurements::Entry& child);

  /** \brief perform a longest prefix match for \p name
   */
  shared_ptr<measurements::Entry>
  findLongestPrefixMatch(const Name& name,
                         const measurements::EntryPredicate& pred =
                             measurements::AnyEntry()) const;

  /** \brief perform a longest prefix match for \p pitEntry.getName()
   */
  shared_ptr<measurements::Entry>
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const measurements::EntryPredicate& pred =
                             measurements::AnyEntry()) const;

  /** \brief perform an exact match
   */
  shared_ptr<measurements::Entry>
  findExactMatch(const Name& name) const;

  static time::nanoseconds
  getInitialLifetime();

  /** \brief extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(measurements::Entry& entry, const time::nanoseconds& lifetime);

  size_t
  size() const;

private:
  void
  cleanup(measurements::Entry& entry);

  shared_ptr<measurements::Entry>
  get(name_tree::Entry& nte);

  /** \tparam K Name or shared_ptr<name_tree::Entry>
   */
  template<typename K>
  shared_ptr<measurements::Entry>
  findLongestPrefixMatchImpl(const K& key, const measurements::EntryPredicate& pred) const;

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

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_HPP
