/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_MEASUREMENTS_ACCESSOR_HPP
#define NFD_TABLE_MEASUREMENTS_ACCESSOR_HPP

#include "measurements.hpp"
#include "fib.hpp"

namespace nfd {

namespace fw {
class Strategy;
}

/** \brief allows Strategy to access portion of Measurements table under its namespace
 */
class MeasurementsAccessor : noncopyable
{
public:
  MeasurementsAccessor(Measurements& measurements, Fib& fib, fw::Strategy* strategy);

  ~MeasurementsAccessor();

  /// find or insert a Measurements entry for name
  shared_ptr<measurements::Entry>
  get(const Name& name);

  /// find or insert a Measurements entry for fibEntry->getPrefix()
  shared_ptr<measurements::Entry>
  get(const fib::Entry& fibEntry);

  /// find or insert a Measurements entry for pitEntry->getName()
  shared_ptr<measurements::Entry>
  get(const pit::Entry& pitEntry);

  /** \brief find or insert a Measurements entry for child's parent
   *
   *  If child is the root entry, returns null.
   */
  shared_ptr<measurements::Entry>
  getParent(shared_ptr<measurements::Entry> child);

//  /// perform a longest prefix match
//  shared_ptr<fib::Entry>
//  findLongestPrefixMatch(const Name& name) const;
//
//  /// perform an exact match
//  shared_ptr<fib::Entry>
//  findExactMatch(const Name& name) const;

  /** \brief extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(measurements::Entry& entry, const time::Duration& lifetime);

private:
  /** \brief perform access control to Measurements entry
   *
   *  \return entry if strategy has access to namespace, otherwise 0
   */
  shared_ptr<measurements::Entry>
  filter(const shared_ptr<measurements::Entry>& entry);

private:
  Measurements& m_measurements;
  Fib& m_fib;
  fw::Strategy* m_strategy;
};

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const Name& name)
{
  return this->filter(m_measurements.get(name));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const fib::Entry& fibEntry)
{
  if (&fibEntry.getStrategy() == m_strategy) {
    return m_measurements.get(fibEntry);
  }
  return shared_ptr<measurements::Entry>();
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::get(const pit::Entry& pitEntry)
{
  return this->filter(m_measurements.get(pitEntry));
}

inline shared_ptr<measurements::Entry>
MeasurementsAccessor::getParent(shared_ptr<measurements::Entry> child)
{
  return this->filter(m_measurements.getParent(child));
}

//inline shared_ptr<fib::Entry>
//MeasurementsAccessor::findLongestPrefixMatch(const Name& name) const
//{
//  return this->filter(m_measurements.findLongestPrefixMatch(name));
//}
//
//inline shared_ptr<fib::Entry>
//MeasurementsAccessor::findExactMatch(const Name& name) const
//{
//  return this->filter(m_measurements.findExactMatch(name));
//}

inline void
MeasurementsAccessor::extendLifetime(measurements::Entry& entry, const time::Duration& lifetime)
{
  m_measurements.extendLifetime(entry, lifetime);
}

} // namespace nfd

#endif // NFD_TABLE_MEASUREMENTS_ACCESSOR_HPP
