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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_HPP

#include "name-tree-iterator.hpp"

namespace nfd {
namespace name_tree {

class NameTree : noncopyable
{
public:
  typedef Iterator const_iterator;

  explicit
  NameTree(size_t nBuckets = 1024);

public: // information
  /** \brief Get the number of occupied entries in the Name Tree
   */
  size_t
  size() const
  {
    return m_ht.size();
  }

  /** \brief Get the number of buckets in the Name Tree (NPHT)
   *
   *  The number of buckets is the one that used to create the hash
   *  table, i.e., m_nBuckets.
   */
  size_t
  getNBuckets() const
  {
    return m_ht.getNBuckets();
  }

  /** \return Name Tree Entry on which a table entry is attached
   */
  template<typename ENTRY>
  shared_ptr<Entry>
  getEntry(const ENTRY& tableEntry) const
  {
    return tableEntry.m_nameTreeEntry.lock();
  }

public: // mutation
  /** \brief Look for the Name Tree Entry that contains this name prefix.
   *
   *  Starts from the shortest name prefix, and then increase the
   *  number of name components by one each time. All non-existing Name Tree
   *  Entries will be created.
   *
   *  \param name The querying name prefix.
   *  \return The pointer to the Name Tree Entry that contains this full name prefix.
   *  \note Existing iterators are unaffected.
   */
  shared_ptr<Entry>
  lookup(const Name& name);

  /** \brief get NameTree entry from FIB entry
   *
   *  This is equivalent to .lookup(fibEntry.getPrefix())
   */
  shared_ptr<Entry>
  lookup(const fib::Entry& fibEntry) const;

  /** \brief get NameTree entry from PIT entry
   *
   *  This is equivalent to .lookup(pitEntry.getName()).
   */
  shared_ptr<Entry>
  lookup(const pit::Entry& pitEntry);

  /** \brief get NameTree entry from Measurements entry
   *
   *  This is equivalent to .lookup(measurementsEntry.getName())
   */
  shared_ptr<Entry>
  lookup(const measurements::Entry& measurementsEntry) const;

  /** \brief get NameTree entry from StrategyChoice entry
   *
   *  This is equivalent to .lookup(strategyChoiceEntry.getName())
   */
  shared_ptr<Entry>
  lookup(const strategy_choice::Entry& strategyChoiceEntry) const;

  /** \brief Delete a Name Tree Entry if this entry is empty.
   *  \param entry The entry to be deleted if empty.
   *  \note This function must be called after a table entry is detached from Name Tree
   *        entry. The function deletes a Name Tree entry if nothing is attached to it and
   *        it has no children, then repeats the same process on its ancestors.
   *  \note Existing iterators, except those pointing to deleted entries, are unaffected.
   */
  size_t
  eraseEntryIfEmpty(Entry* entry);

  /// \deprecated
  bool
  eraseEntryIfEmpty(shared_ptr<Entry> entry)
  {
    BOOST_ASSERT(entry != nullptr);
    return this->eraseEntryIfEmpty(entry.get()) > 0;
  }

public: // matching
  /** \brief Exact match lookup for the given name prefix.
   *  \return nullptr if this prefix is not found; otherwise return the Name Tree Entry address
   */
  shared_ptr<Entry>
  findExactMatch(const Name& name) const;

  /** \brief Longest prefix matching for the given name
   *
   *  Starts from the full name string, reduce the number of name component
   *  by one each time, until an Entry is found.
   */
  shared_ptr<Entry>
  findLongestPrefixMatch(const Name& name,
                         const EntrySelector& entrySelector = AnyEntry()) const;

  shared_ptr<Entry>
  findLongestPrefixMatch(shared_ptr<Entry> entry,
                         const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief longest prefix matching for Interest name of the PIT entry
   *
   *  This is equivalent to .findLongestPrefixMatch(pitEntry.getName(), AnyEntry()).
   */
  shared_ptr<Entry>
  findLongestPrefixMatch(const pit::Entry& pitEntry) const;

  /** \brief Enumerate all the name prefixes that satisfy the prefix and entrySelector
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& allMatches = nt.findAllMatches(Name("/A/B/C"));
   *  for (const Entry& nte : allMatches) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  findAllMatches(const Name& prefix,
                 const EntrySelector& entrySelector = AnyEntry()) const;

public: // enumeration
  /** \brief Enumerate all entries, optionally filtered by an EntrySelector.
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& enumerable = nt.fullEnumerate();
   *  for (const Entry& nte : enumerable) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  fullEnumerate(const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief Enumerate all entries under a prefix, optionally filtered by an EntrySubTreeSelector.
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& enumerable = nt.partialEnumerate(Name("/A/B/C"));
   *  for (const Entry& nte : enumerable) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  partialEnumerate(const Name& prefix,
                   const EntrySubTreeSelector& entrySubTreeSelector = AnyEntrySubTree()) const;

  /** \brief Get an iterator pointing to the first NameTree entry
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  const_iterator
  begin() const
  {
    return fullEnumerate().begin();
  }

  /** \brief Get an iterator referring to the past-the-end FIB entry
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  const_iterator
  end() const
  {
    return Iterator();
  }

private:
  Hashtable m_ht;

  friend class EnumerationImpl;
};

} // namespace name_tree

using name_tree::NameTree;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_NAME_TREE_HPP
