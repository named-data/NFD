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

#ifndef NFD_DAEMON_TABLE_NAME_TREE_HPP
#define NFD_DAEMON_TABLE_NAME_TREE_HPP

#include "common.hpp"
#include "name-tree-entry.hpp"

namespace nfd {
namespace name_tree {

/**
 * \brief Compute the hash value of the given name prefix's WIRE FORMAT
 */
size_t
computeHash(const Name& prefix);

/**
 * \brief Incrementally compute hash values
 * \return Return a vector of hash values, starting from the root prefix
 */
std::vector<size_t>
computeHashSet(const Name& prefix);

/// a predicate to accept or reject an Entry in find operations
typedef function<bool (const Entry& entry)> EntrySelector;

/**
 * \brief a predicate to accept or reject an Entry and its children
 * \return .first indicates whether entry should be accepted;
 *         .second indicates whether entry's children should be visited
 */
typedef function<std::pair<bool,bool> (const Entry& entry)> EntrySubTreeSelector;

struct AnyEntry {
  bool
  operator()(const Entry& entry)
  {
    return true;
  }
};

struct AnyEntrySubTree {
  std::pair<bool, bool>
  operator()(const Entry& entry)
  {
    return std::make_pair(true, true);
  }
};

} // namespace name_tree

/**
 * \brief Class Name Tree
 */
class NameTree : noncopyable
{
public:
  class const_iterator;

  explicit
  NameTree(size_t nBuckets = 1024);

  ~NameTree();

public: // information
  /**
   * \brief Get the number of occupied entries in the Name Tree
   */
  size_t
  size() const;

  /**
   * \brief Get the number of buckets in the Name Tree (NPHT)
   * \details The number of buckets is the one that used to create the hash
   * table, i.e., m_nBuckets.
   */
  size_t
  getNBuckets() const;

  /**
   * \brief Dump all the information stored in the Name Tree for debugging.
   */
  void
  dump(std::ostream& output) const;

public: // mutation
  /**
   * \brief Look for the Name Tree Entry that contains this name prefix.
   * \details Starts from the shortest name prefix, and then increase the
   * number of name components by one each time. All non-existing Name Tree
   * Entries will be created.
   * \param prefix The querying name prefix.
   * \return The pointer to the Name Tree Entry that contains this full name
   * prefix.
   * \note Existing iterators are unaffected.
   */
  shared_ptr<name_tree::Entry>
  lookup(const Name& prefix);

  /**
   * \brief Delete a Name Tree Entry if this entry is empty.
   * \param entry The entry to be deleted if empty.
   * \note This function must be called after a table entry is detached from Name Tree
   *       entry. The function deletes a Name Tree entry if nothing is attached to it and
   *       it has no children, then repeats the same process on its ancestors.
   * \note Existing iterators, except those pointing to deleted entries, are unaffected.
   */
  bool
  eraseEntryIfEmpty(shared_ptr<name_tree::Entry> entry);

public: // shortcut access
  /// get NameTree entry from attached FIB entry
  shared_ptr<name_tree::Entry>
  get(const fib::Entry& fibEntry) const;

  /// get NameTree entry from attached PIT entry
  shared_ptr<name_tree::Entry>
  get(const pit::Entry& pitEntry) const;

  /// get NameTree entry from attached Measurements entry
  shared_ptr<name_tree::Entry>
  get(const measurements::Entry& measurementsEntry) const;

  /// get NameTree entry from attached StrategyChoice entry
  shared_ptr<name_tree::Entry>
  get(const strategy_choice::Entry& strategyChoiceEntry) const;

public: // matching
  /**
   * \brief Exact match lookup for the given name prefix.
   * \return a null shared_ptr if this prefix is not found;
   * otherwise return the Name Tree Entry address
   */
  shared_ptr<name_tree::Entry>
  findExactMatch(const Name& prefix) const;

  /**
   * \brief Longest prefix matching for the given name
   * \details Starts from the full name string, reduce the number of name component
   * by one each time, until an Entry is found.
   */
  shared_ptr<name_tree::Entry>
  findLongestPrefixMatch(const Name& prefix,
                         const name_tree::EntrySelector& entrySelector =
                         name_tree::AnyEntry()) const;

  shared_ptr<name_tree::Entry>
  findLongestPrefixMatch(shared_ptr<name_tree::Entry> entry,
                         const name_tree::EntrySelector& entrySelector =
                         name_tree::AnyEntry()) const;

  /** \brief Enumerate all the name prefixes that satisfy the prefix and entrySelector
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& allMatches = nt.findAllMatches(Name("/A/B/C"));
   *  for (const name_tree::Entry& nte : allMatches) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  findAllMatches(const Name& prefix,
                 const name_tree::EntrySelector& entrySelector = name_tree::AnyEntry()) const;

public: // enumeration
  /** \brief Enumerate all entries, optionally filtered by an EntrySelector.
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& enumerable = nt.fullEnumerate();
   *  for (const name_tree::Entry& nte : enumerable) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  fullEnumerate(const name_tree::EntrySelector& entrySelector = name_tree::AnyEntry()) const;

  /** \brief Enumerate all entries under a prefix, optionally filtered by an EntrySubTreeSelector.
   *  \return an unspecified type that have .begin() and .end() methods
   *          and is usable with range-based for
   *
   *  Example:
   *  \code{.cpp}
   *  auto&& enumerable = nt.partialEnumerate(Name("/A/B/C"));
   *  for (const name_tree::Entry& nte : enumerable) {
   *    ...
   *  }
   *  \endcode
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  boost::iterator_range<const_iterator>
  partialEnumerate(const Name& prefix,
                   const name_tree::EntrySubTreeSelector& entrySubTreeSelector =
                         name_tree::AnyEntrySubTree()) const;

  /** \brief Get an iterator pointing to the first NameTree entry
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  const_iterator
  begin() const;

  /** \brief Get an iterator referring to the past-the-end FIB entry
   *  \note Iteration order is implementation-specific and is undefined
   *  \note The returned iterator may get invalidated when NameTree is modified
   */
  const_iterator
  end() const;

  enum IteratorType {
    FULL_ENUMERATE_TYPE,
    PARTIAL_ENUMERATE_TYPE,
    FIND_ALL_MATCHES_TYPE
  };

  class const_iterator : public std::iterator<std::forward_iterator_tag, const name_tree::Entry>
  {
  public:
    friend class NameTree;

    const_iterator();

    const_iterator(NameTree::IteratorType type,
      const NameTree& nameTree,
      shared_ptr<name_tree::Entry> entry,
      const name_tree::EntrySelector& entrySelector = name_tree::AnyEntry(),
      const name_tree::EntrySubTreeSelector& entrySubTreeSelector = name_tree::AnyEntrySubTree());

    ~const_iterator();

    const name_tree::Entry&
    operator*() const;

    shared_ptr<name_tree::Entry>
    operator->() const;

    const_iterator
    operator++();

    const_iterator
    operator++(int);

    bool
    operator==(const const_iterator& other) const;

    bool
    operator!=(const const_iterator& other) const;

  private:
    const NameTree*                             m_nameTree;
    shared_ptr<name_tree::Entry>                m_entry;
    shared_ptr<name_tree::Entry>                m_subTreeRoot;
    shared_ptr<name_tree::EntrySelector>        m_entrySelector;
    shared_ptr<name_tree::EntrySubTreeSelector> m_entrySubTreeSelector;
    NameTree::IteratorType                      m_type;
    bool                                        m_shouldVisitChildren;
  };

private:
  /**
   * \brief Resize the hash table size when its load factor reaches a threshold.
   * \details As we are currently using a hand-written hash table implementation
   * for the Name Tree, the hash table resize() function should be kept in the
   * name-tree.hpp file.
   * \param newNBuckets The number of buckets for the new hash table.
   */
  void
  resize(size_t newNBuckets);

private:
  size_t                        m_nItems;  // Number of items being stored
  size_t                        m_nBuckets; // Number of hash buckets
  size_t                        m_minNBuckets; // Minimum number of hash buckets
  double                        m_enlargeLoadFactor;
  size_t                        m_enlargeThreshold;
  int                           m_enlargeFactor;
  double                        m_shrinkLoadFactor;
  size_t                        m_shrinkThreshold;
  double                        m_shrinkFactor;
  name_tree::Node**             m_buckets; // Name Tree Buckets in the NPHT
  shared_ptr<name_tree::Entry>  m_end;
  const_iterator                m_endIterator;

  /**
   * \brief Create a Name Tree Entry if it does not exist, or return the existing
   * Name Tree Entry address.
   * \details Called by lookup() only.
   * \return The first item is the Name Tree Entry address, the second item is
   * a bool value indicates whether this is an old entry (false) or a new
   * entry (true).
   */
  std::pair<shared_ptr<name_tree::Entry>, bool>
  insert(const Name& prefix);
};

inline NameTree::const_iterator::~const_iterator()
{
}

inline size_t
NameTree::size() const
{
  return m_nItems;
}

inline size_t
NameTree::getNBuckets() const
{
  return m_nBuckets;
}

inline shared_ptr<name_tree::Entry>
NameTree::get(const fib::Entry& fibEntry) const
{
  return fibEntry.m_nameTreeEntry;
}

inline shared_ptr<name_tree::Entry>
NameTree::get(const pit::Entry& pitEntry) const
{
  return pitEntry.m_nameTreeEntry;
}

inline shared_ptr<name_tree::Entry>
NameTree::get(const measurements::Entry& measurementsEntry) const
{
  return measurementsEntry.m_nameTreeEntry;
}

inline shared_ptr<name_tree::Entry>
NameTree::get(const strategy_choice::Entry& strategyChoiceEntry) const
{
  return strategyChoiceEntry.m_nameTreeEntry;
}

inline NameTree::const_iterator
NameTree::begin() const
{
  return fullEnumerate().begin();
}

inline NameTree::const_iterator
NameTree::end() const
{
  return m_endIterator;
}

inline const name_tree::Entry&
NameTree::const_iterator::operator*() const
{
  return *m_entry;
}

inline shared_ptr<name_tree::Entry>
NameTree::const_iterator::operator->() const
{
  return m_entry;
}

inline NameTree::const_iterator
NameTree::const_iterator::operator++(int)
{
  NameTree::const_iterator temp(*this);
  ++(*this);
  return temp;
}

inline bool
NameTree::const_iterator::operator==(const NameTree::const_iterator& other) const
{
  return m_entry == other.m_entry;
}

inline bool
NameTree::const_iterator::operator!=(const NameTree::const_iterator& other) const
{
  return m_entry != other.m_entry;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_NAME_TREE_HPP
