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

#include "strategy-choice.hpp"
#include "core/logger.hpp"
#include "fw/strategy.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

namespace nfd {

using strategy_choice::Entry;
using fw::Strategy;

NFD_LOG_INIT("StrategyChoice");

StrategyChoice::StrategyChoice(NameTree& nameTree, shared_ptr<Strategy> defaultStrategy)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
  this->setDefaultStrategy(defaultStrategy);
}

bool
StrategyChoice::hasStrategy(const Name& strategyName, bool isExact) const
{
  if (isExact) {
    return m_strategyInstances.count(strategyName) > 0;
  }
  else {
    return static_cast<bool>(this->getStrategy(strategyName));
  }
}

bool
StrategyChoice::install(shared_ptr<Strategy> strategy)
{
  BOOST_ASSERT(static_cast<bool>(strategy));
  const Name& strategyName = strategy->getName();

  if (this->hasStrategy(strategyName, true)) {
    NFD_LOG_ERROR("install(" << strategyName << ") duplicate strategyName");
    return false;
  }

  m_strategyInstances[strategyName] = strategy;
  return true;
}

fw::Strategy*
StrategyChoice::getStrategy(const Name& strategyName) const
{
  fw::Strategy* candidate = nullptr;
  for (auto it = m_strategyInstances.lower_bound(strategyName);
       it != m_strategyInstances.end() && strategyName.isPrefixOf(it->first); ++it) {
    switch (it->first.size() - strategyName.size()) {
    case 0: // exact match
      return it->second.get();
    case 1: // unversioned strategyName matches versioned strategy
      candidate = it->second.get();
      break;
    }
  }
  return candidate;
}

bool
StrategyChoice::insert(const Name& prefix, const Name& strategyName)
{
  Strategy* strategy = this->getStrategy(strategyName);
  if (strategy == nullptr) {
    NFD_LOG_ERROR("insert(" << prefix << "," << strategyName << ") strategy not installed");
    return false;
  }

  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(prefix);
  shared_ptr<Entry> entry = nte->getStrategyChoiceEntry();
  Strategy* oldStrategy = nullptr;
  if (static_cast<bool>(entry)) {
    if (entry->getStrategy().getName() == strategy->getName()) {
      NFD_LOG_TRACE("insert(" << prefix << ") not changing " << strategy->getName());
      return true;
    }
    oldStrategy = &entry->getStrategy();
    NFD_LOG_TRACE("insert(" << prefix << ") changing from " << oldStrategy->getName() <<
                  " to " << strategy->getName());
  }

  if (!static_cast<bool>(entry)) {
    oldStrategy = &this->findEffectiveStrategy(prefix);
    entry = make_shared<Entry>(prefix);
    nte->setStrategyChoiceEntry(entry);
    ++m_nItems;
    NFD_LOG_TRACE("insert(" << prefix << ") new entry " << strategy->getName());
  }

  this->changeStrategy(*entry, *oldStrategy, *strategy);
  entry->setStrategy(*strategy);
  return true;
}

void
StrategyChoice::erase(const Name& prefix)
{
  BOOST_ASSERT(prefix.size() > 0);

  shared_ptr<name_tree::Entry> nte = m_nameTree.findExactMatch(prefix);
  if (!static_cast<bool>(nte)) {
    return;
  }

  shared_ptr<Entry> entry = nte->getStrategyChoiceEntry();
  if (!static_cast<bool>(entry)) {
    return;
  }

  Strategy& oldStrategy = entry->getStrategy();

  Strategy& parentStrategy = this->findEffectiveStrategy(prefix.getPrefix(-1));
  this->changeStrategy(*entry, oldStrategy, parentStrategy);

  nte->setStrategyChoiceEntry(shared_ptr<Entry>());
  m_nameTree.eraseEntryIfEmpty(nte);
  --m_nItems;
}

std::pair<bool, Name>
StrategyChoice::get(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findExactMatch(prefix);
  if (!static_cast<bool>(nte)) {
    return { false, Name() };
  }

  shared_ptr<Entry> entry = nte->getStrategyChoiceEntry();
  if (!static_cast<bool>(entry)) {
    return { false, Name() };
  }

  return { true, entry->getStrategy().getName() };
}

Strategy&
StrategyChoice::findEffectiveStrategy(const Name& prefix) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.findLongestPrefixMatch(prefix,
    [] (const name_tree::Entry& entry) {
      return static_cast<bool>(entry.getStrategyChoiceEntry());
    });

  BOOST_ASSERT(static_cast<bool>(nte));
  return nte->getStrategyChoiceEntry()->getStrategy();
}

Strategy&
StrategyChoice::findEffectiveStrategy(shared_ptr<name_tree::Entry> nte) const
{
  shared_ptr<strategy_choice::Entry> entry = nte->getStrategyChoiceEntry();
  if (static_cast<bool>(entry))
    return entry->getStrategy();

  nte = m_nameTree.findLongestPrefixMatch(nte,
    [] (const name_tree::Entry& entry) {
      return static_cast<bool>(entry.getStrategyChoiceEntry());
    });

  BOOST_ASSERT(static_cast<bool>(nte));
  return nte->getStrategyChoiceEntry()->getStrategy();
}

Strategy&
StrategyChoice::findEffectiveStrategy(const pit::Entry& pitEntry) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(pitEntry);

  BOOST_ASSERT(static_cast<bool>(nte));
  return this->findEffectiveStrategy(nte);
}

Strategy&
StrategyChoice::findEffectiveStrategy(const measurements::Entry& measurementsEntry) const
{
  shared_ptr<name_tree::Entry> nte = m_nameTree.get(measurementsEntry);

  BOOST_ASSERT(static_cast<bool>(nte));
  return this->findEffectiveStrategy(nte);
}

void
StrategyChoice::setDefaultStrategy(shared_ptr<Strategy> strategy)
{
  this->install(strategy);

  // don't use .insert here, because it will invoke findEffectiveStrategy
  // which expects an existing root entry
  shared_ptr<name_tree::Entry> nte = m_nameTree.lookup(Name());
  shared_ptr<Entry> entry = make_shared<Entry>(Name());
  nte->setStrategyChoiceEntry(entry);
  ++m_nItems;
  NFD_LOG_INFO("setDefaultStrategy " << strategy->getName());

  entry->setStrategy(*strategy);
}

static inline void
clearStrategyInfo(const name_tree::Entry& nte)
{
  NFD_LOG_TRACE("clearStrategyInfo " << nte.getPrefix());

  for (const shared_ptr<pit::Entry>& pitEntry : nte.getPitEntries()) {
    pitEntry->clearStrategyInfo();
    for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
      const_cast<pit::InRecord&>(inRecord).clearStrategyInfo();
    }
    for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
      const_cast<pit::OutRecord&>(outRecord).clearStrategyInfo();
    }
  }
  if (static_cast<bool>(nte.getMeasurementsEntry())) {
    nte.getMeasurementsEntry()->clearStrategyInfo();
  }
}

void
StrategyChoice::changeStrategy(strategy_choice::Entry& entry,
                               fw::Strategy& oldStrategy,
                               fw::Strategy& newStrategy)
{
  if (&oldStrategy == &newStrategy) {
    return;
  }

  NFD_LOG_INFO("changeStrategy(" << entry.getPrefix() << ")"
               << " from " << oldStrategy.getName()
               << " to " << newStrategy.getName());

  // reset StrategyInfo on a portion of NameTree,
  // where entry's effective strategy is covered by the changing StrategyChoice entry
  const name_tree::Entry* rootNte = m_nameTree.get(entry).get();
  auto&& ntChanged = m_nameTree.partialEnumerate(entry.getPrefix(),
    [&rootNte] (const name_tree::Entry& nte) -> std::pair<bool, bool> {
      if (&nte == rootNte) {
        return {true, true};
      }
      if (static_cast<bool>(nte.getStrategyChoiceEntry())) {
        return {false, false};
      }
      return {true, true};
    });
  for (const name_tree::Entry& nte : ntChanged) {
    clearStrategyInfo(nte);
  }
}

StrategyChoice::const_iterator
StrategyChoice::begin() const
{
  auto&& enumerable = m_nameTree.fullEnumerate(
    [] (const name_tree::Entry& entry) {
      return static_cast<bool>(entry.getStrategyChoiceEntry());
    });
  return const_iterator(enumerable.begin());
}

} // namespace nfd
