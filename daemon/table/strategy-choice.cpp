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

#include "strategy-choice.hpp"
#include "core/logger.hpp"
#include "fw/strategy.hpp"
#include "pit-entry.hpp"
#include "measurements-entry.hpp"

namespace nfd {
namespace strategy_choice {

using fw::Strategy;

NFD_LOG_INIT("StrategyChoice");

static inline bool
nteHasStrategyChoiceEntry(const name_tree::Entry& nte)
{
  return nte.getStrategyChoiceEntry() != nullptr;
}

StrategyChoice::StrategyChoice(NameTree& nameTree, unique_ptr<Strategy> defaultStrategy)
  : m_nameTree(nameTree)
  , m_nItems(0)
{
  this->setDefaultStrategy(std::move(defaultStrategy));
}

bool
StrategyChoice::hasStrategy(const Name& strategyName, bool isExact) const
{
  if (isExact) {
    return m_strategyInstances.count(strategyName) > 0;
  }
  else {
    return this->getStrategy(strategyName) != nullptr;
  }
}

std::pair<bool, Strategy*>
StrategyChoice::install(unique_ptr<Strategy> strategy)
{
  BOOST_ASSERT(strategy != nullptr);
  Name strategyName = strategy->getName();
  // copying Name, so that strategyName remains available even if strategy is deallocated

  bool isInserted = false;
  StrategyInstanceTable::iterator it;
  std::tie(it, isInserted) = m_strategyInstances.emplace(strategyName, std::move(strategy));

  if (!isInserted) {
    NFD_LOG_ERROR("install(" << strategyName << ") duplicate strategyName");
  }
  return std::make_pair(isInserted, it->second.get());
}

Strategy*
StrategyChoice::getStrategy(const Name& strategyName) const
{
  Strategy* candidate = nullptr;
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

  name_tree::Entry& nte = m_nameTree.lookup(prefix);
  Entry* entry = nte.getStrategyChoiceEntry();
  Strategy* oldStrategy = nullptr;
  if (entry != nullptr) {
    if (entry->getStrategy().getName() == strategy->getName()) {
      NFD_LOG_TRACE("insert(" << prefix << ") not changing " << strategy->getName());
      return true;
    }
    oldStrategy = &entry->getStrategy();
    NFD_LOG_TRACE("insert(" << prefix << ") changing from " << oldStrategy->getName() <<
                  " to " << strategy->getName());
  }

  if (entry == nullptr) {
    oldStrategy = &this->findEffectiveStrategy(prefix);
    auto newEntry = make_unique<Entry>(prefix);
    entry = newEntry.get();
    nte.setStrategyChoiceEntry(std::move(newEntry));
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

  name_tree::Entry* nte = m_nameTree.findExactMatch(prefix);
  if (nte == nullptr) {
    return;
  }

  Entry* entry = nte->getStrategyChoiceEntry();
  if (entry == nullptr) {
    return;
  }

  Strategy& oldStrategy = entry->getStrategy();

  Strategy& parentStrategy = this->findEffectiveStrategy(prefix.getPrefix(-1));
  this->changeStrategy(*entry, oldStrategy, parentStrategy);

  nte->setStrategyChoiceEntry(nullptr);
  m_nameTree.eraseIfEmpty(nte);
  --m_nItems;
}

std::pair<bool, Name>
StrategyChoice::get(const Name& prefix) const
{
  name_tree::Entry* nte = m_nameTree.findExactMatch(prefix);
  if (nte == nullptr) {
    return {false, Name()};
  }

  Entry* entry = nte->getStrategyChoiceEntry();
  if (entry == nullptr) {
    return {false, Name()};
  }

  return {true, entry->getStrategy().getName()};
}

template<typename K>
Strategy&
StrategyChoice::findEffectiveStrategyImpl(const K& key) const
{
  const name_tree::Entry* nte = m_nameTree.findLongestPrefixMatch(key, &nteHasStrategyChoiceEntry);
  BOOST_ASSERT(nte != nullptr);
  return nte->getStrategyChoiceEntry()->getStrategy();
}

Strategy&
StrategyChoice::findEffectiveStrategy(const Name& prefix) const
{
  return this->findEffectiveStrategyImpl(prefix);
}

Strategy&
StrategyChoice::findEffectiveStrategy(const pit::Entry& pitEntry) const
{
  return this->findEffectiveStrategyImpl(pitEntry);
}

Strategy&
StrategyChoice::findEffectiveStrategy(const measurements::Entry& measurementsEntry) const
{
  return this->findEffectiveStrategyImpl(measurementsEntry);
}

void
StrategyChoice::setDefaultStrategy(unique_ptr<Strategy> strategy)
{
  bool isInstalled = false;
  Strategy* instance = nullptr;
  std::tie(isInstalled, instance) = this->install(std::move(strategy));
  BOOST_ASSERT(isInstalled);

  auto entry = make_unique<Entry>(Name());
  entry->setStrategy(*instance);

  // don't use .insert here, because it will invoke findEffectiveStrategy
  // which expects an existing root entry
  name_tree::Entry& nte = m_nameTree.lookup(Name());
  nte.setStrategyChoiceEntry(std::move(entry));
  ++m_nItems;
  NFD_LOG_INFO("setDefaultStrategy " << instance->getName());
}

static inline void
clearStrategyInfo(const name_tree::Entry& nte)
{
  NFD_LOG_TRACE("clearStrategyInfo " << nte.getName());

  for (const shared_ptr<pit::Entry>& pitEntry : nte.getPitEntries()) {
    pitEntry->clearStrategyInfo();
    for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
      const_cast<pit::InRecord&>(inRecord).clearStrategyInfo();
    }
    for (const pit::OutRecord& outRecord : pitEntry->getOutRecords()) {
      const_cast<pit::OutRecord&>(outRecord).clearStrategyInfo();
    }
  }
  if (nte.getMeasurementsEntry() != nullptr) {
    nte.getMeasurementsEntry()->clearStrategyInfo();
  }
}

void
StrategyChoice::changeStrategy(Entry& entry, Strategy& oldStrategy, Strategy& newStrategy)
{
  if (&oldStrategy == &newStrategy) {
    return;
  }

  NFD_LOG_INFO("changeStrategy(" << entry.getPrefix() << ")"
               << " from " << oldStrategy.getName()
               << " to " << newStrategy.getName());

  // reset StrategyInfo on a portion of NameTree,
  // where entry's effective strategy is covered by the changing StrategyChoice entry
  const name_tree::Entry* rootNte = m_nameTree.getEntry(entry);
  BOOST_ASSERT(rootNte != nullptr);
  auto&& ntChanged = m_nameTree.partialEnumerate(entry.getPrefix(),
    [&rootNte] (const name_tree::Entry& nte) -> std::pair<bool, bool> {
      if (&nte == rootNte) {
        return {true, true};
      }
      if (nte.getStrategyChoiceEntry() != nullptr) {
        return {false, false};
      }
      return {true, true};
    });
  for (const name_tree::Entry& nte : ntChanged) {
    clearStrategyInfo(nte);
  }
}

StrategyChoice::Range
StrategyChoice::getRange() const
{
  return m_nameTree.fullEnumerate(&nteHasStrategyChoiceEntry) |
         boost::adaptors::transformed(name_tree::GetTableEntry<Entry>(
                                      &name_tree::Entry::getStrategyChoiceEntry));
}

} // namespace strategy_choice
} // namespace nfd
