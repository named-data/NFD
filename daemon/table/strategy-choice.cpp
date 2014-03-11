/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#include "strategy-choice.hpp"
#include "fw/strategy.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

namespace nfd {

using strategy_choice::Entry;
using fw::Strategy;

NFD_LOG_INIT("StrategyChoice");

StrategyChoice::StrategyChoice(NameTree& nameTree, shared_ptr<Strategy> defaultStrategy)
  : m_nameTree(nameTree)
{
  this->setDefaultStrategy(defaultStrategy);
}

bool
StrategyChoice::hasStrategy(const Name& strategyName) const
{
  return m_strategyInstances.count(strategyName) > 0;
}

bool
StrategyChoice::install(shared_ptr<Strategy> strategy)
{
  BOOST_ASSERT(static_cast<bool>(strategy));
  const Name& strategyName = strategy->getName();

  if (this->hasStrategy(strategyName)) {
    NFD_LOG_ERROR("install(" << strategyName << ") duplicate strategyName");
    return false;
  }

  m_strategyInstances[strategyName] = strategy;
  return true;
}

bool
StrategyChoice::insert(const Name& prefix, const Name& strategyName)
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(prefix);
  shared_ptr<Entry> entry = nameTreeEntry->getStrategyChoiceEntry();
  shared_ptr<Strategy> oldStrategy;

  if (static_cast<bool>(entry)) {
    if (entry->getStrategy().getName() == strategyName) {
      NFD_LOG_INFO("insert(" << prefix << "," << strategyName << ") not changing");
      return true;
    }
    oldStrategy = entry->getStrategy().shared_from_this();
    NFD_LOG_INFO("insert(" << prefix << "," << strategyName << ") "
                 "changing from " << oldStrategy->getName());
  }

  shared_ptr<Strategy> strategy = this->getStrategy(strategyName);
  if (!static_cast<bool>(strategy)) {
    NFD_LOG_ERROR("insert(" << prefix << "," << strategyName << ") strategy not installed");
    return false;
  }

  if (!static_cast<bool>(entry)) {
    oldStrategy = this->findEffectiveStrategy(prefix).shared_from_this();
    entry = make_shared<Entry>(prefix);
    nameTreeEntry->setStrategyChoiceEntry(entry);
    ++m_nItems;
    NFD_LOG_INFO("insert(" << prefix << "," << strategyName << ") new entry");
  }

  this->changeStrategy(entry, oldStrategy, strategy);
  return true;
}

void
StrategyChoice::erase(const Name& prefix)
{
  BOOST_ASSERT(prefix.size() > 0);

  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(prefix);
  if (!static_cast<bool>(nameTreeEntry)) {
    return;
  }

  shared_ptr<Entry> entry = nameTreeEntry->getStrategyChoiceEntry();
  if (!static_cast<bool>(entry)) {
    return;
  }

  Strategy& oldStrategy = entry->getStrategy();
  nameTreeEntry->setStrategyChoiceEntry(shared_ptr<Entry>());
  --m_nItems;

  Strategy& parentStrategy = this->findEffectiveStrategy(prefix);
  this->changeStrategy(entry, oldStrategy.shared_from_this(), parentStrategy.shared_from_this());
}

shared_ptr<const Name>
StrategyChoice::get(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.findExactMatch(prefix);
  if (!static_cast<bool>(nameTreeEntry)) {
    return shared_ptr<const Name>();
  }

  shared_ptr<Entry> entry = nameTreeEntry->getStrategyChoiceEntry();
  if (!static_cast<bool>(entry)) {
    return shared_ptr<const Name>();
  }

  return entry->getStrategy().getName().shared_from_this();
}

static inline bool
predicate_NameTreeEntry_hasStrategyChoiceEntry(const name_tree::Entry& entry)
{
  return static_cast<bool>(entry.getStrategyChoiceEntry());
}

Strategy&
StrategyChoice::findEffectiveStrategy(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry =
    m_nameTree.findLongestPrefixMatch(prefix, &predicate_NameTreeEntry_hasStrategyChoiceEntry);
  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));
  return nameTreeEntry->getStrategyChoiceEntry()->getStrategy();
}

Strategy&
StrategyChoice::findEffectiveStrategy(shared_ptr<name_tree::Entry> nameTreeEntry) const
{
  shared_ptr<strategy_choice::Entry> entry = nameTreeEntry->getStrategyChoiceEntry();
  if (static_cast<bool>(entry))
    return entry->getStrategy();
  nameTreeEntry = m_nameTree.findLongestPrefixMatch(nameTreeEntry, 
                               &predicate_NameTreeEntry_hasStrategyChoiceEntry);
  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));
  return nameTreeEntry->getStrategyChoiceEntry()->getStrategy();
}

Strategy&
StrategyChoice::findEffectiveStrategy(const pit::Entry& pitEntry) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(pitEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return findEffectiveStrategy(nameTreeEntry);
}

Strategy&
StrategyChoice::findEffectiveStrategy(const measurements::Entry& measurementsEntry) const
{
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.get(measurementsEntry);

  BOOST_ASSERT(static_cast<bool>(nameTreeEntry));

  return findEffectiveStrategy(nameTreeEntry);
}

shared_ptr<fw::Strategy>
StrategyChoice::getStrategy(const Name& strategyName)
{
  StrategyInstanceTable::iterator it = m_strategyInstances.find(strategyName);
  return it != m_strategyInstances.end() ? it->second : shared_ptr<fw::Strategy>();
}

void
StrategyChoice::setDefaultStrategy(shared_ptr<Strategy> strategy)
{
  this->install(strategy);

  // don't use .insert here, because it will invoke findEffectiveStrategy
  // which expects an existing root entry
  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(Name());
  shared_ptr<Entry> entry = make_shared<Entry>(Name());
  nameTreeEntry->setStrategyChoiceEntry(entry);
  ++m_nItems;
  NFD_LOG_INFO("setDefaultStrategy(" << strategy->getName() << ") new entry");

  entry->setStrategy(strategy);
}

void
StrategyChoice::changeStrategy(shared_ptr<strategy_choice::Entry> entry,
                               shared_ptr<fw::Strategy> oldStrategy,
                               shared_ptr<fw::Strategy> newStrategy)
{
  entry->setStrategy(newStrategy);
  if (oldStrategy == newStrategy) {
    return;
  }

  // TODO delete incompatible StrategyInfo
  NFD_LOG_WARN("changeStrategy(" << entry->getPrefix() << ") " <<
               "runtime strategy change not implemented");
}

} // namespace nfd
