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

#ifndef NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP
#define NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP

#include "measurements.hpp"
#include "strategy-choice.hpp"

namespace nfd {

namespace fw {
class Strategy;
} // namespace fw

namespace measurements {

/** \brief allows Strategy to access portion of Measurements table under its namespace
 *
 *  All public methods have the same semantics as the same method on \p Measurements,
 *  but would return nullptr if the entry falls out of the strategy's authority.
 */
class MeasurementsAccessor : noncopyable
{
public:
  MeasurementsAccessor(Measurements& measurements, const StrategyChoice& strategyChoice,
                       const fw::Strategy& strategy);

  ~MeasurementsAccessor();

  /** \brief find or insert a Measurements entry for \p name
   */
  Entry*
  get(const Name& name);

  /** \brief find or insert a Measurements entry for \p fibEntry->getPrefix()
   */
  Entry*
  get(const fib::Entry& fibEntry);

  /** \brief find or insert a Measurements entry for \p pitEntry->getName()
   */
  Entry*
  get(const pit::Entry& pitEntry);

  /** \brief find or insert a Measurements entry for child's parent
   */
  Entry*
  getParent(const Entry& child);

  /** \brief perform a longest prefix match for \p name
   */
  Entry*
  findLongestPrefixMatch(const Name& name,
                         const EntryPredicate& pred =
                             AnyEntry()) const;

  /** \brief perform a longest prefix match for \p pitEntry.getName()
   */
  Entry*
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const EntryPredicate& pred =
                             AnyEntry()) const;

  /** \brief perform an exact match
   */
  Entry*
  findExactMatch(const Name& name) const;

  /** \brief extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(Entry& entry, const time::nanoseconds& lifetime);

private:
  /** \brief perform access control to Measurements entry
   *  \return entry if strategy has access to namespace, otherwise nullptr
   */
  Entry*
  filter(Entry* entry) const;

  Entry*
  filter(Entry& entry) const;

private:
  Measurements& m_measurements;
  const StrategyChoice& m_strategyChoice;
  const fw::Strategy* m_strategy;
};

inline Entry*
MeasurementsAccessor::filter(Entry& entry) const
{
  return this->filter(&entry);
}

inline Entry*
MeasurementsAccessor::get(const Name& name)
{
  return this->filter(m_measurements.get(name));
}

inline Entry*
MeasurementsAccessor::get(const fib::Entry& fibEntry)
{
  return this->filter(m_measurements.get(fibEntry));
}

inline Entry*
MeasurementsAccessor::get(const pit::Entry& pitEntry)
{
  return this->filter(m_measurements.get(pitEntry));
}

inline Entry*
MeasurementsAccessor::getParent(const Entry& child)
{
  return this->filter(m_measurements.getParent(child));
}

inline Entry*
MeasurementsAccessor::findLongestPrefixMatch(const Name& name,
                                             const EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(name, pred));
}

inline Entry*
MeasurementsAccessor::findLongestPrefixMatch(const pit::Entry& pitEntry,
                                             const EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(pitEntry, pred));
}

inline Entry*
MeasurementsAccessor::findExactMatch(const Name& name) const
{
  return this->filter(m_measurements.findExactMatch(name));
}

inline void
MeasurementsAccessor::extendLifetime(Entry& entry, const time::nanoseconds& lifetime)
{
  m_measurements.extendLifetime(entry, lifetime);
}

} // namespace measurements

using measurements::MeasurementsAccessor;

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP
