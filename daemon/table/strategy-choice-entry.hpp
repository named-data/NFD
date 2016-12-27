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

#ifndef NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP
#define NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP

#include "core/common.hpp"

namespace nfd {

namespace fw {
class Strategy;
} // namespace fw

namespace name_tree {
class Entry;
} // namespace name_tree

namespace strategy_choice {

/** \brief represents a Strategy Choice entry
 */
class Entry : noncopyable
{
public:
  Entry(const Name& prefix);

  ~Entry();

  /** \return name prefix on which this strategy choice is applied
   */
  const Name&
  getPrefix() const
  {
    return m_prefix;
  }

  /** \return strategy instance name
   */
  const Name&
  getStrategyInstanceName() const;

  /** \return strategy instance
   */
  fw::Strategy&
  getStrategy() const
  {
    BOOST_ASSERT(m_strategy != nullptr);
    return *m_strategy;
  }

  void
  setStrategy(unique_ptr<fw::Strategy> strategy);

private:
  Name m_prefix;
  unique_ptr<fw::Strategy> m_strategy;

  name_tree::Entry* m_nameTreeEntry;
  friend class name_tree::Entry;
};

} // namespace strategy_choice
} // namespace nfd

#endif // NFD_DAEMON_TABLE_STRATEGY_CHOICE_ENTRY_HPP
