/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_MEASUREMENTS_HPP
#define NFD_TABLE_MEASUREMENTS_HPP

#include "measurements-entry.hpp"
#include "fib-entry.hpp"
#include "pit-entry.hpp"
#include "core/time.hpp"

namespace nfd {

/** \class Measurement
 *  \brief represents the Measurements table
 */
class Measurements : noncopyable
{
public:
  explicit
  Measurements(boost::asio::io_service& ioService);

  ~Measurements();

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
  void
  extendLifetimeInternal(
    std::map<Name, shared_ptr<measurements::Entry> >::iterator it,
    const time::Duration& lifetime);

  void
  cleanup(std::map<Name, shared_ptr<measurements::Entry> >::iterator it);

private:
  std::map<Name, shared_ptr<measurements::Entry> > m_table;

  Scheduler m_scheduler;
  static const time::Duration s_defaultLifetime;
};

} // namespace nfd

#endif // NFD_TABLE_MEASUREMENTS_HPP
