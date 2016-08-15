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

#ifndef NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP
#define NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP

#include "strategy-choice-entry.hpp"
#include "name-tree.hpp"

#include <boost/range/adaptor/transformed.hpp>

namespace nfd {
namespace strategy_choice {

/** \brief represents the Strategy Choice table
 *
 *  The Strategy Choice table maintains available Strategy types,
 *  and associates Name prefixes with Strategy types.
 *
 *  Each strategy is identified by a strategyName.
 *  It's recommended to include a version number as the last component of strategyName.
 *
 *  A Name prefix is owned by a strategy if a longest prefix match on the
 *  Strategy Choice table returns that strategy.
 */
class StrategyChoice : noncopyable
{
public:
  StrategyChoice(NameTree& nameTree, unique_ptr<fw::Strategy> defaultStrategy);

  size_t
  size() const
  {
    return m_nItems;
  }

public: // available Strategy types
  /** \brief determines if a strategy is installed
   *  \param strategyName name of the strategy
   *  \param isExact true to require exact match, false to permit unversioned strategyName
   *  \return true if strategy is installed
   */
  bool
  hasStrategy(const Name& strategyName, bool isExact = false) const;

  /** \brief install a strategy
   *  \return if installed, true, and a pointer to the strategy instance;
   *          if not installed due to duplicate strategyName, false,
   *          and a pointer to the existing strategy instance
   */
  std::pair<bool, fw::Strategy*>
  install(unique_ptr<fw::Strategy> strategy);

public: // Strategy Choice table
  /** \brief set strategy of prefix to be strategyName
   *  \param prefix the name prefix for which \p strategyName should be used
   *  \param strategyName the strategy to be used
   *  \return true on success
   *
   *  This method set a strategy onto a Name prefix.
   *  The strategy must have been installed.
   *  The strategyName can either be exact (contains version component),
   *  or omit the version component to pick the latest version.
   */
  bool
  insert(const Name& prefix, const Name& strategyName);

  /** \brief make prefix to inherit strategy from its parent
   *
   *  not allowed for root prefix (ndn:/)
   */
  void
  erase(const Name& prefix);

  /** \brief get strategy Name of prefix
   *  \return true and strategyName at exact match, or false
   */
  std::pair<bool, Name>
  get(const Name& prefix) const;

public: // effective strategy
  /** \brief get effective strategy for prefix
   */
  fw::Strategy&
  findEffectiveStrategy(const Name& prefix) const;

  /** \brief get effective strategy for pitEntry
   *
   *  This is equivalent to .findEffectiveStrategy(pitEntry.getName())
   */
  fw::Strategy&
  findEffectiveStrategy(const pit::Entry& pitEntry) const;

  /** \brief get effective strategy for measurementsEntry
   *
   *  This is equivalent to .findEffectiveStrategy(measurementsEntry.getName())
   */
  fw::Strategy&
  findEffectiveStrategy(const measurements::Entry& measurementsEntry) const;

public: // enumeration
  typedef boost::transformed_range<name_tree::GetTableEntry<Entry>, const name_tree::Range> Range;
  typedef boost::range_iterator<Range>::type const_iterator;

  /** \return an iterator to the beginning
   *  \note Iteration order is implementation-defined.
   *  \warning Undefined behavior may occur if a FIB/PIT/Measurements/StrategyChoice entry
   *           is inserted or erased during enumeration.
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

private:
  /** \brief get Strategy instance by strategyName
   *  \param strategyName a versioned or unversioned strategyName
   */
  fw::Strategy*
  getStrategy(const Name& strategyName) const;

  void
  setDefaultStrategy(unique_ptr<fw::Strategy> strategy);

  void
  changeStrategy(Entry& entry,
                 fw::Strategy& oldStrategy,
                 fw::Strategy& newStrategy);

  /** \tparam K a parameter acceptable to NameTree::findLongestPrefixMatch
   */
  template<typename K>
  fw::Strategy&
  findEffectiveStrategyImpl(const K& key) const;

  Range
  getRange() const;

private:
  NameTree& m_nameTree;
  size_t m_nItems;

  typedef std::map<Name, unique_ptr<fw::Strategy>> StrategyInstanceTable;
  StrategyInstanceTable m_strategyInstances;
};

} // namespace strategy_choice

using strategy_choice::StrategyChoice;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP
