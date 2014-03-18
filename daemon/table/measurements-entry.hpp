/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_MEASUREMENTS_ENTRY_HPP
#define NFD_TABLE_MEASUREMENTS_ENTRY_HPP

#include "common.hpp"
#include "strategy-info-host.hpp"
#include "core/scheduler.hpp"

namespace nfd {

class NameTree;

namespace name_tree {
class Entry;
}

class Measurements;

namespace measurements {

/** \class Entry
 *  \brief represents a Measurements entry
 */
class Entry : public StrategyInfoHost, noncopyable
{
public:
  explicit
  Entry(const Name& name);

  const Name&
  getName() const;

private:
  Name m_name;

private: // lifetime
  time::steady_clock::TimePoint m_expiry;
  EventId m_cleanup;
  shared_ptr<name_tree::Entry> m_nameTreeEntry;

  friend class nfd::NameTree;
  friend class nfd::name_tree::Entry;
  friend class nfd::Measurements;
};

inline const Name&
Entry::getName() const
{
  return m_name;
}

} // namespace measurements
} // namespace nfd

#endif // NFD_TABLE_MEASUREMENTS_ENTRY_HPP
