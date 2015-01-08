/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2014-2015,  Regents of the University of California,
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
}

/** \brief allows Strategy to access portion of Measurements table under its namespace
 */
class MeasurementsAccessor : noncopyable
{
public:
  MeasurementsAccessor(Measurements& measurements, const StrategyChoice& strategyChoice,
                       const fw::Strategy& strategy);

  ~MeasurementsAccessor();

  /** \brief find or insert a Measurements entry for \p name
   */
  shared_ptr<measurements::Entry>
  get(const Name& name);

  /** \brief find or insert a Measurements entry for \p fibEntry->getPrefix()
   */
  shared_ptr<measurements::Entry>
  get(const fib::Entry& fibEntry);

  /** \brief find or insert a Measurements entry for \p pitEntry->getName()
   */
  shared_ptr<measurements::Entry>
  get(const pit::Entry& pitEntry);

  /** \brief find or insert a Measurements entry for child's parent
   *  \retval nullptr if child is the root entry
   */
  shared_ptr<measurements::Entry>
  getParent(const measurements::Entry& child);

  /** \brief perform a longest prefix match for \p name
   */
  shared_ptr<measurements::Entry>
  findLongestPrefixMatch(const Name& name,
                         const measurements::EntryPredicate& pred =
                             measurements::AnyEntry()) const;

  /** \brief perform a longest prefix match for \p pitEntry.getName()
   */
  shared_ptr<measurements::Entry>
  findLongestPrefixMatch(const pit::Entry& pitEntry,
                         const measurements::EntryPredicate& pred =
                             measurements::AnyEntry()) const;

  /** \brief perform an exact match
   */
  shared_ptr<measurements::Entry>
  findExactMatch(const Name& name) const;

  /** \brief extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(measurements::Entry& entry, const time::nanoseconds& lifetime);

private:
  /** \brief perform access control to Measurements entry
   *  \return entry if strategy has access to namespace, otherwise nullptr
   */
  shared_ptr<measurements::Entry>
  filter(const shared_ptr<measurements::Entry>& entry) const;

private:
  Measurements& m_measurements;
  const StrategyChoice& m_strategyChoice;
  const fw::Strategy* m_strategy;
};

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const Name& name)
{
  return this->filter(m_measurements.get(name));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const fib::Entry& fibEntry)
{
  return this->filter(m_measurements.get(fibEntry));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const pit::Entry& pitEntry)
{
  return this->filter(m_measurements.get(pitEntry));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::getParent(const measurements::Entry& child)
{
  return this->filter(m_measurements.getParent(child));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::findLongestPrefixMatch(const Name& name,
                                             const measurements::EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(name, pred));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::findLongestPrefixMatch(const pit::Entry& pitEntry,
                                             const measurements::EntryPredicate& pred) const
{
  return this->filter(m_measurements.findLongestPrefixMatch(pitEntry, pred));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::findExactMatch(const Name& name) const
{
  return this->filter(m_measurements.findExactMatch(name));
}

inline void
MeasurementsAccessor::extendLifetime(measurements::Entry& entry,
                                     const time::nanoseconds& lifetime)
{
  m_measurements.extendLifetime(entry, lifetime);
}

} // namespace nfd

#endif // NFD_DAEMON_TABLE_MEASUREMENTS_ACCESSOR_HPP
