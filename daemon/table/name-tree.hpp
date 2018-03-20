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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_HPP

#include "name-tree-iterator.hpp"

#include "core/fib-max-depth.hpp"

namespace nfd {
namespace name_tree {

/** \brief a common index structure for FIB, PIT, StrategyChoice, and Measurements
 */
class NameTree : noncopyable
{
public:
  explicit
  NameTree(size_t nBuckets = 1024);

public: // information
  /** \brief maximum depth of the name tree
   *
   *  Calling \c NameTree::lookup with a name with many components would cause the creation of many
   *  NameTree entries, which could take very long time. This constant limits the maximum number of
   *  name components in the name of a NameTree entry. Thus, it limits the number of NameTree
   *  entries created from a long name, bounding the processing complexity.
   */
  static constexpr size_t
  getMaxDepth()
  {
    return FIB_MAX_DEPTH;
  }

  /** \return number of name tree entries
   */
  size_t
  size() const
  {
    return m_ht.size();
  }

  /** \return number of hashtable buckets
   */
  size_t
  getNBuckets() const
  {
    return m_ht.getNBuckets();
  }

  /** \return name tree entry on which a table entry is attached,
   *          or nullptr if the table entry is detached
   */
  template<typename EntryT>
  Entry*
  getEntry(const EntryT& tableEntry) const
  {
    return Entry::get(tableEntry);
  }

public: // mutation
  /** \brief find or insert an entry by name
   *
   *  This method seeks a name tree entry of name \c name.getPrefix(prefixLen).
   *  If the entry does not exist, it is created along with all ancestors.
   *  Existing iterators are unaffected during this operation.
   *
   *  \warning \p prefixLen must not exceed \c name.size().
   *  \warning \p prefixLen must not exceed \c getMaxDepth().
   */
  Entry&
  lookup(const Name& name, size_t prefixLen);

  /** \brief equivalent to `lookup(name, name.size())`
   */
  Entry&
  lookup(const Name& name)
  {
    return this->lookup(name, name.size());
  }

  /** \brief equivalent to `lookup(fibEntry.getPrefix())`
   *  \param fibEntry a FIB entry attached to this name tree, or \c Fib::s_emptyEntry
   *  \note This overload is more efficient than `lookup(const Name&)` in common cases.
   */
  Entry&
  lookup(const fib::Entry& fibEntry);

  /** \brief equivalent to
   *         `lookup(pitEntry.getName(), std::min(pitEntry.getName().size(), getMaxDepth()))`
   *  \param pitEntry a PIT entry attached to this name tree
   *  \note This overload is more efficient than `lookup(const Name&)` in common cases.
   */
  Entry&
  lookup(const pit::Entry& pitEntry);

  /** \brief equivalent to `lookup(measurementsEntry.getName())`
   *  \param measurementsEntry a Measurements entry attached to this name tree
   *  \note This overload is more efficient than `lookup(const Name&)` in common cases.
   */
  Entry&
  lookup(const measurements::Entry& measurementsEntry);

  /** \brief equivalent to `lookup(strategyChoiceEntry.getPrefix())`
   *  \param strategyChoiceEntry a StrategyChoice entry attached to this name tree
   *  \note This overload is more efficient than `lookup(const Name&)` in common cases.
   */
  Entry&
  lookup(const strategy_choice::Entry& strategyChoiceEntry);

  /** \brief delete the entry if it is empty
   *  \param entry a valid entry
   *  \param canEraseAncestors whether ancestors should be deleted if they become empty
   *  \return number of deleted entries
   *  \sa Entry::isEmpty()
   *  \post If the entry is empty, it's deleted. If \p canEraseAncestors is true,
   *        ancestors of the entry are also deleted if they become empty.
   *  \note This function must be called after detaching a table entry from a name tree entry,
   *  \note Existing iterators, except those pointing to deleted entries, are unaffected.
   */
  size_t
  eraseIfEmpty(Entry* entry, bool canEraseAncestors = true);

public: // matching
  /** \brief exact match lookup
   *  \return entry with \c name.getPrefix(prefixLen), or nullptr if it does not exist
   */
  Entry*
  findExactMatch(const Name& name, size_t prefixLen = std::numeric_limits<size_t>::max()) const;

  /** \brief longest prefix matching
   *  \return entry whose name is a prefix of \p name and passes \p entrySelector,
   *          where no other entry with a longer name satisfies those requirements;
   *          or nullptr if no entry satisfying those requirements exists
   */
  Entry*
  findLongestPrefixMatch(const Name& name,
                         const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief equivalent to `findLongestPrefixMatch(entry.getName(), entrySelector)`
   *  \note This overload is more efficient than
   *        `findLongestPrefixMatch(const Name&, const EntrySelector&)` in common cases.
   */
  Entry*
  findLongestPrefixMatch(const Entry& entry,
                         const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief equivalent to `findLongestPrefixMatch(getEntry(tableEntry)->getName(), entrySelector)`
   *  \tparam EntryT \c fib::Entry or \c measurements::Entry or \c strategy_choice::Entry
   *  \note This overload is more efficient than
   *        `findLongestPrefixMatch(const Name&, const EntrySelector&)` in common cases.
   *  \warning Undefined behavior may occur if \p tableEntry is not attached to this name tree.
   */
  template<typename EntryT>
  Entry*
  findLongestPrefixMatch(const EntryT& tableEntry,
                         const EntrySelector& entrySelector = AnyEntry()) const
  {
    const Entry* nte = this->getEntry(tableEntry);
    BOOST_ASSERT(nte != nullptr);
    return this->findLongestPrefixMatch(*nte, entrySelector);
  }

  /** \brief equivalent to `findLongestPrefixMatch(pitEntry.getName(), entrySelector)`
   *  \note This overload is more efficient than
   *        `findLongestPrefixMatch(const Name&, const EntrySelector&)` in common cases.
   *  \warning Undefined behavior may occur if \p pitEntry is not attached to this name tree.
   */
  Entry*
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief all-prefixes match lookup
   *  \return a range where every entry has a name that is a prefix of \p name ,
   *          and matches \p entrySelector.
   *
   *  Example:
   *  \code
   *  Name name("/A/B/C");
   *  auto&& allMatches = nt.findAllMatches(name);
   *  for (const Entry& nte : allMatches) {
   *    BOOST_ASSERT(nte.getName().isPrefixOf(name));
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-defined.
   *  \warning If a name tree entry whose name is a prefix of \p name is inserted
   *           during the enumeration, it may or may not be visited.
   *           If a name tree entry whose name is a prefix of \p name is deleted
   *           during the enumeration, undefined behavior may occur.
   */
  Range
  findAllMatches(const Name& name,
                 const EntrySelector& entrySelector = AnyEntry()) const;

public: // enumeration
  using const_iterator = Iterator;

  /** \brief enumerate all entries
   *  \return a range where every entry matches \p entrySelector
   *
   *  Example:
   *  \code
   *  auto&& enumerable = nt.fullEnumerate();
   *  for (const Entry& nte : enumerable) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-defined.
   *  \warning If a name tree entry is inserted or deleted during the enumeration,
   *           it may cause the enumeration to skip entries or visit some entries twice.
   */
  Range
  fullEnumerate(const EntrySelector& entrySelector = AnyEntry()) const;

  /** \brief enumerate all entries under a prefix
   *  \return a range where every entry has a name that starts with \p prefix,
   *          and matches \p entrySubTreeSelector.
   *
   *  Example:
   *  \code
   *  Name name("/A/B/C");
   *  auto&& enumerable = nt.partialEnumerate(name);
   *  for (const Entry& nte : enumerable) {
   *    BOOST_ASSERT(name.isPrefixOf(nte.getName()));
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-defined.
   *  \warning If a name tree entry under \p prefix is inserted or deleted during the enumeration,
   *           it may cause the enumeration to skip entries or visit some entries twice.
   */
  Range
  partialEnumerate(const Name& prefix,
                   const EntrySubTreeSelector& entrySubTreeSelector = AnyEntrySubTree()) const;

  /** \return an iterator to the beginning
   *  \sa fullEnumerate
   */
  const_iterator
  begin() const
  {
    return fullEnumerate().begin();
  }

  /** \return an iterator to the end
   *  \sa begin()
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
