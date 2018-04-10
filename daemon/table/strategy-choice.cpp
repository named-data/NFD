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

#include "strategy-choice.hpp"
#include "measurements-entry.hpp"
#include "pit-entry.hpp"
#include "core/logger.hpp"
#include "fw/strategy.hpp"

#include <ndn-cxx/util/concepts.hpp>

namespace nfd {
namespace strategy_choice {

NDN_CXX_ASSERT_FORWARD_ITERATOR(StrategyChoice::const_iterator);

NFD_LOG_INIT("StrategyChoice");

using fw::Strategy;

static inline bool
nteHasStrategyChoiceEntry(const name_tree::Entry& nte)
{
  return nte.getStrategyChoiceEntry() != nullptr;
}

StrategyChoice::StrategyChoice(Forwarder& forwarder)
  : m_forwarder(forwarder)
  , m_nameTree(m_forwarder.getNameTree())
  , m_nItems(0)
{
}

void
StrategyChoice::setDefaultStrategy(const Name& strategyName)
{
  auto entry = make_unique<Entry>(Name());
  entry->setStrategy(Strategy::create(strategyName, m_forwarder));
  NFD_LOG_INFO("setDefaultStrategy " << entry->getStrategyInstanceName());

  // don't use .insert here, because it will invoke findEffectiveStrategy
  // which expects an existing root entry
  name_tree::Entry& nte = m_nameTree.lookup(Name());
  nte.setStrategyChoiceEntry(std::move(entry));
  ++m_nItems;
}

StrategyChoice::InsertResult
StrategyChoice::insert(const Name& prefix, const Name& strategyName)
{
  if (prefix.size() > NameTree::getMaxDepth()) {
    return InsertResult::DEPTH_EXCEEDED;
  }

  unique_ptr<Strategy> strategy;
  try {
    strategy = Strategy::create(strategyName, m_forwarder);
  }
  catch (const std::invalid_argument& e) {
    NFD_LOG_ERROR("insert(" << prefix << "," << strategyName << ") cannot create strategy: " << e.what());
    return InsertResult(InsertResult::EXCEPTION, e.what());
  }

  if (strategy == nullptr) {
    NFD_LOG_ERROR("insert(" << prefix << "," << strategyName << ") strategy not registered");
    return InsertResult::NOT_REGISTERED;
  }

  name_tree::Entry& nte = m_nameTree.lookup(prefix);
  Entry* entry = nte.getStrategyChoiceEntry();
  Strategy* oldStrategy = nullptr;
  if (entry != nullptr) {
    if (entry->getStrategyInstanceName() == strategy->getInstanceName()) {
      NFD_LOG_TRACE("insert(" << prefix << ") not changing " << strategy->getInstanceName());
      return InsertResult::OK;
    }
    oldStrategy = &entry->getStrategy();
    NFD_LOG_TRACE("insert(" << prefix << ") changing from " << oldStrategy->getInstanceName() <<
                  " to " << strategy->getInstanceName());
  }
  else {
    oldStrategy = &this->findEffectiveStrategy(prefix);
    auto newEntry = make_unique<Entry>(prefix);
    entry = newEntry.get();
    nte.setStrategyChoiceEntry(std::move(newEntry));
    ++m_nItems;
    NFD_LOG_TRACE("insert(" << prefix << ") new entry " << strategy->getInstanceName());
  }

  this->changeStrategy(*entry, *oldStrategy, *strategy);
  entry->setStrategy(std::move(strategy));
  return InsertResult::OK;
}

StrategyChoice::InsertResult::InsertResult(Status status, const std::string& exceptionMessage)
  : m_status(status)
  , m_exceptionMessage(exceptionMessage)
{
}

std::ostream&
operator<<(std::ostream& os, const StrategyChoice::InsertResult& res)
{
  switch (res.m_status) {
    case StrategyChoice::InsertResult::OK:
      return os << "OK";
    case StrategyChoice::InsertResult::NOT_REGISTERED:
      return os << "Strategy not registered";
    case StrategyChoice::InsertResult::EXCEPTION:
      return os << "Error instantiating strategy: " << res.m_exceptionMessage;
    case StrategyChoice::InsertResult::DEPTH_EXCEEDED:
      return os << "Prefix has too many components (limit is "
                 << to_string(NameTree::getMaxDepth()) << ")";
  }
  return os;
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

  return {true, entry->getStrategyInstanceName()};
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
  const Name& oldInstanceName = oldStrategy.getInstanceName();
  const Name& newInstanceName = newStrategy.getInstanceName();
  if (Strategy::areSameType(oldInstanceName, newInstanceName)) {
    // same Strategy subclass type: no need to clear StrategyInfo
    NFD_LOG_INFO("changeStrategy(" << entry.getPrefix() << ") "
                 << oldInstanceName << " -> " << newInstanceName
                 << " same-type");
    return;
  }

  NFD_LOG_INFO("changeStrategy(" << entry.getPrefix() << ") "
               << oldInstanceName << " -> " << newInstanceName);

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
