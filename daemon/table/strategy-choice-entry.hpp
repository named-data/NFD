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
 **/

#ifndef NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP
#define NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP

#include "common.hpp"

namespace nfd {

class NameTree;
namespace name_tree {
class Entry;
}
namespace fw {
class Strategy;
}

namespace strategy_choice {

/** \brief represents a Strategy Choice entry
 */
class Entry : noncopyable
{
public:
  Entry(const Name& prefix);

  const Name&
  getPrefix() const;

  const Name&
  getStrategyName() const;

  fw::Strategy&
  getStrategy() const;

  void
  setStrategy(fw::Strategy& strategy);

private:
  Name m_prefix;
  fw::Strategy* m_strategy;

  shared_ptr<name_tree::Entry> m_nameTreeEntry;
  friend class nfd::NameTree;
  friend class nfd::name_tree::Entry;
};


inline const Name&
Entry::getPrefix() const
{
  return m_prefix;
}

inline fw::Strategy&
Entry::getStrategy() const
{
  BOOST_ASSERT(m_strategy != nullptr);
  return *m_strategy;
}

inline void
Entry::setStrategy(fw::Strategy& strategy)
{
  m_strategy = &strategy;
}

} // namespace strategy_choice
} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP
