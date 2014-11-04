/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014,  Regents of the University of California,
 *                      Arizona Board of Regents,
 *                      Colorado State University,
 *                      University Pierre & Marie Curie, Sorbonne University,
 *                      Washington University in St. Louis,
 *                      Beijing Institute of Technology,
 *                      The University of Memphis
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

namespace nfd {

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
  StrategyChoice(NameTree& nameTree, shared_ptr<fw::Strategy> defaultStrategy);

public: // available Strategy types
  /** \brief determines if a strategy is installed
   *  \param isExact true to require exact match, false to permit unversioned strategyName
   *  \return true if strategy is installed
   */
  bool
  hasStrategy(const Name& strategyName, bool isExact = false) const;

  /** \brief install a strategy
   *  \return true if installed; false if not installed due to duplicate strategyName
   *  \note shared_ptr is passed by value because StrategyChoice takes ownership of strategy
   */
  bool
  install(shared_ptr<fw::Strategy> strategy);

public: // Strategy Choice table
  /** \brief set strategy of prefix to be strategyName
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
  /// get effective strategy for prefix
  fw::Strategy&
  findEffectiveStrategy(const Name& prefix) const;

  /// get effective strategy for pitEntry
  fw::Strategy&
  findEffectiveStrategy(const pit::Entry& pitEntry) const;

  /// get effective strategy for measurementsEntry
  fw::Strategy&
  findEffectiveStrategy(const measurements::Entry& measurementsEntry) const;

public: // enumeration
  class const_iterator
    : public std::iterator<std::forward_iterator_tag, const strategy_choice::Entry>
  {
  public:
    explicit
    const_iterator(const NameTree::const_iterator& it);

    ~const_iterator();

    const strategy_choice::Entry&
    operator*() const;

    shared_ptr<strategy_choice::Entry>
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

  /// number of entries stored
  size_t
  size() const;

  const_iterator
  begin() const;

  const_iterator
  end() const;

private:
  /** \brief get Strategy instance by strategyName
   *  \param strategyName a versioned or unversioned strategyName
   */
  fw::Strategy*
  getStrategy(const Name& strategyName) const;

  void
  setDefaultStrategy(shared_ptr<fw::Strategy> strategy);

  void
  changeStrategy(strategy_choice::Entry& entry,
                 fw::Strategy& oldStrategy,
                 fw::Strategy& newStrategy);

  fw::Strategy&
  findEffectiveStrategy(shared_ptr<name_tree::Entry> nte) const;

private:
  NameTree& m_nameTree;
  size_t m_nItems;

  typedef std::map<Name, shared_ptr<fw::Strategy> > StrategyInstanceTable;
  StrategyInstanceTable m_strategyInstances;
};

inline size_t
StrategyChoice::size() const
{
  return m_nItems;
}

inline StrategyChoice::const_iterator
StrategyChoice::end() const
{
  return const_iterator(m_nameTree.end());
}

inline
StrategyChoice::const_iterator::const_iterator(const NameTree::const_iterator& it)
  : m_nameTreeIterator(it)
{
}

inline
StrategyChoice::const_iterator::~const_iterator()
{
}

inline
StrategyChoice::const_iterator
StrategyChoice::const_iterator::operator++(int)
{
  StrategyChoice::const_iterator temp(*this);
  ++(*this);
  return temp;
}

inline StrategyChoice::const_iterator&
StrategyChoice::const_iterator::operator++()
{
  ++m_nameTreeIterator;
  return *this;
}

inline const strategy_choice::Entry&
StrategyChoice::const_iterator::operator*() const
{
  return *(m_nameTreeIterator->getStrategyChoiceEntry());
}

inline shared_ptr<strategy_choice::Entry>
StrategyChoice::const_iterator::operator->() const
{
  return m_nameTreeIterator->getStrategyChoiceEntry();
}

inline bool
StrategyChoice::const_iterator::operator==(const StrategyChoice::const_iterator& other) const
{
  return m_nameTreeIterator == other.m_nameTreeIterator;
}

inline bool
StrategyChoice::const_iterator::operator!=(const StrategyChoice::const_iterator& other) const
{
  return m_nameTreeIterator != other.m_nameTreeIterator;
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_CHOICE_HPP
