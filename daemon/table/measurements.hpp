/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_MEASUREMENTS_HPP
#define NFD_TABLE_MEASUREMENTS_HPP

#include "measurements-entry.hpp"
#include "core/time.hpp"
#include "name-tree.hpp"

namespace nfd {

namespace fib {
class Entry;
}

namespace pit {
class Entry;
}


/** \class Measurement
 *  \brief represents the Measurements table
 */
class Measurements : noncopyable
{
public:
  explicit
  Measurements(NameTree& nametree);

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

  /** find or insert a Measurements entry for child's parent
   *
   *  If child is the root entry, returns null.
   */
  shared_ptr<measurements::Entry>
  getParent(shared_ptr<measurements::Entry> child);

  /// perform a longest prefix match
  shared_ptr<measurements::Entry>
  findLongestPrefixMatch(const Name& name) const;

  /// perform an exact match
  shared_ptr<measurements::Entry>
  findExactMatch(const Name& name) const;

  /** extend lifetime of an entry
   *
   *  The entry will be kept until at least now()+lifetime.
   */
  void
  extendLifetime(measurements::Entry& entry, const time::Duration lifetime);

  size_t
  size() const;

private:
  void
  cleanup(shared_ptr<measurements::Entry> entry);

  shared_ptr<measurements::Entry>
  get(shared_ptr<name_tree::Entry> nameTreeEntry);

private:
  NameTree& m_nameTree;
  size_t m_nItems;
  static const time::Duration s_defaultLifetime;
};

inline size_t
Measurements::size() const
{
  return m_nItems;
}

} // namespace nfd

#endif // NFD_TABLE_MEASUREMENTS_HPP
