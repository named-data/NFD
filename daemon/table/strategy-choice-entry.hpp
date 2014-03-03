/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NFD_TABLE_STRATEGY_CHOICE_ENTRY_HPP
#define NFD_TABLE_STRATEGY_CHOICE_ENTRY_HPP

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

  fw::Strategy&
  getStrategy() const;

  void
  setStrategy(shared_ptr<fw::Strategy> strategy);

private:
  Name m_prefix;
  shared_ptr<fw::Strategy> m_strategy;

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
  BOOST_ASSERT(static_cast<bool>(m_strategy));
  return *m_strategy;
}

} // namespace strategy_choice
} // namespace nfd

#endif // NFD_TABLE_STRATEGY_CHOICE_ENTRY_HPP
