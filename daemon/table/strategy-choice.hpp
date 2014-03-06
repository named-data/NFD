/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_STRATEGY_CHOICE_HPP
#define NFD_TABLE_STRATEGY_CHOICE_HPP

#include "strategy-choice-entry.hpp"
#include "name-tree.hpp"

namespace nfd {

class StrategyChoice : noncopyable
{
public:
  StrategyChoice(NameTree& nameTree, shared_ptr<fw::Strategy> defaultStrategy);

public: // available Strategy types
  /** \return true if strategy is installed
   */
  bool
  hasStrategy(const Name& strategyName) const;

  /** \brief install a strategy
   *  \return true if installed; false if not installed due to duplicate strategyName
   */
  bool
  install(shared_ptr<fw::Strategy> strategy);

public: // Strategy Choice table
  /** \brief set strategy of prefix to be strategyName
   *  \param strategyName the strategy to be used, must be installed
   *  \return true on success
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
   *  \return strategyName at exact match, or nullptr
   */
  shared_ptr<const Name>
  get(const Name& prefix) const;

public: // effect strategy
  /// get effective strategy for prefix
  fw::Strategy&
  findEffectiveStrategy(const Name& prefix) const;

  /// get effective strategy for pitEntry
  fw::Strategy&
  findEffectiveStrategy(const pit::Entry& pitEntry) const;

  /// get effective strategy for measurementsEntry
  fw::Strategy&
  findEffectiveStrategy(const measurements::Entry& measurementsEntry) const;

  /// number of entries stored
  size_t
  size() const;

private:
  shared_ptr<fw::Strategy>
  getStrategy(const Name& strategyName);

  void
  setDefaultStrategy(shared_ptr<fw::Strategy> strategy);

  void
  changeStrategy(shared_ptr<strategy_choice::Entry> entry,
                 shared_ptr<fw::Strategy> oldStrategy,
                 shared_ptr<fw::Strategy> newStrategy);

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

} // namespace nfd

#endif // NFD_TABLE_STRATEGY_CHOICE_HPP
