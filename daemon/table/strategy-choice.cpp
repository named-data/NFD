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

  if (this->hasStrategy(strategyName)) {
    NFD_LOG_ERROR("install(" << strategyName << ") duplicate strategyName");
    return false;
  }

  m_strategyInstances[strategyName] = strategy;
  return true;
}

shared_ptr<fw::Strategy>
StrategyChoice::getStrategy(const Name& strategyName) const
{
  shared_ptr<fw::Strategy> candidate;
  for (StrategyInstanceTable::const_iterator it = m_strategyInstances.lower_bound(strategyName);
       it != m_strategyInstances.end() && strategyName.isPrefixOf(it->first); ++it) {
    switch (it->first.size() - strategyName.size()) {
    case 0: // exact match
      return it->second;
    case 1: // unversioned strategyName matches versioned strategy
      candidate = it->second;
      break;
    }
  }
  return candidate;
}

bool
StrategyChoice::insert(const Name& prefix, const Name& strategyName)
{
  shared_ptr<Strategy> strategy = this->getStrategy(strategyName);
  if (!static_cast<bool>(strategy)) {
    NFD_LOG_ERROR("insert(" << prefix << "," << strategyName << ") strategy not installed");
    return false;
  }

  shared_ptr<name_tree::Entry> nameTreeEntry = m_nameTree.lookup(prefix);
  shared_ptr<Entry> entry = nameTreeEntry->getStrategyChoiceEntry();
  shared_ptr<Strategy> oldStrategy;
  if (static_cast<bool>(entry)) {
    if (entry->getStrategy().getName() == strategy->getName()) {
      NFD_LOG_TRACE("insert(" << prefix << ") not changing " << strategy->getName());
      return true;
    }
    oldStrategy = entry->getStrategy().shared_from_this();
    NFD_LOG_TRACE("insert(" << prefix << ") changing from " << oldStrategy->getName() <<
                  " to " << strategy->getName());
  }

  if (!static_cast<bool>(entry)) {
    oldStrategy = this->findEffectiveStrategy(prefix).shared_from_this();
    entry = make_shared<Entry>(prefix);
    nameTreeEntry->setStrategyChoiceEntry(entry);
    ++m_nItems;
    NFD_LOG_TRACE("insert(" << prefix << ") new entry " << strategy->getName());
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

  Strategy& parentStrategy = this->findEffectiveStrategy(prefix.getPrefix(-1));
  this->changeStrategy(entry, oldStrategy.shared_from_this(), parentStrategy.shared_from_this());

  nameTreeEntry->setStrategyChoiceEntry(shared_ptr<Entry>());
  m_nameTree.eraseEntryIfEmpty(nameTreeEntry);
  --m_nItems;
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

  return make_shared<Name>(entry->getStrategy().getName());
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
  NFD_LOG_INFO("Set default strategy " << strategy->getName());

  entry->setStrategy(strategy);
}

/** \brief a predicate that decides whether StrategyInfo should be reset
 *
 *  StrategyInfo on a NameTree entry needs to be reset,
 *  if its effective strategy is covered by the changing StrategyChoice entry.
 */
static inline std::pair<bool,bool>
predicate_nameTreeEntry_needResetStrategyChoice(const name_tree::Entry& nameTreeEntry,
                                            const name_tree::Entry& rootEntry)
{
  if (&nameTreeEntry == &rootEntry) {
    return std::make_pair(true, true);
  }
  if (static_cast<bool>(nameTreeEntry.getStrategyChoiceEntry())) {
    return std::make_pair(false, false);
  }
  return std::make_pair(true, true);
}

static inline void
clearStrategyInfo_pitFaceRecord(const pit::FaceRecord& pitFaceRecord)
{
  const_cast<pit::FaceRecord&>(pitFaceRecord).clearStrategyInfo();
}

static inline void
clearStrategyInfo_pitEntry(shared_ptr<pit::Entry> pitEntry)
{
  pitEntry->clearStrategyInfo();
  std::for_each(pitEntry->getInRecords().begin(), pitEntry->getInRecords().end(),
                &clearStrategyInfo_pitFaceRecord);
  std::for_each(pitEntry->getOutRecords().begin(), pitEntry->getOutRecords().end(),
                &clearStrategyInfo_pitFaceRecord);
}

static inline void
clearStrategyInfo(const name_tree::Entry& nameTreeEntry)
{
  NFD_LOG_TRACE("clearStrategyInfo " << nameTreeEntry.getPrefix());

  std::for_each(nameTreeEntry.getPitEntries().begin(), nameTreeEntry.getPitEntries().end(),
                &clearStrategyInfo_pitEntry);
  if (static_cast<bool>(nameTreeEntry.getMeasurementsEntry())) {
    nameTreeEntry.getMeasurementsEntry()->clearStrategyInfo();
  }
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

  std::for_each(m_nameTree.partialEnumerate(entry->getPrefix(),
                           bind(&predicate_nameTreeEntry_needResetStrategyChoice,
                                _1, cref(*m_nameTree.get(*entry)))),
                m_nameTree.end(),
                &clearStrategyInfo);
}

StrategyChoice::const_iterator
StrategyChoice::begin() const
{
  return const_iterator(m_nameTree.fullEnumerate(&predicate_NameTreeEntry_hasStrategyChoiceEntry));
}

} // namespace nfd
