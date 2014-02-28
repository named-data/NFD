/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (C) 2014 Named Data Networking Project
 * See COPYING for copyright and distribution information.
 */

#ifndef NRD_RIB_HPP
#define NRD_RIB_HPP

#include "common.hpp"

namespace ndn {
namespace nrd {

/** \brief represents the RIB
 */
class Rib
{
public:
  typedef std::list<PrefixRegOptions> RibTable;
  typedef RibTable::const_iterator const_iterator;

  Rib();

  ~Rib();

  const_iterator
  find(const PrefixRegOptions& options) const;

  void
  insert(const PrefixRegOptions& options);

  void
  erase(const PrefixRegOptions& options);

  const_iterator
  begin() const;

  const_iterator
  end() const;

  size_t
  size() const;

  size_t
  empty() const;

private:
  // Note: Taking a list right now. A Trie will be used later to store
  // prefixes
  RibTable m_rib;
};

inline Rib::const_iterator
Rib::begin() const
{
  return m_rib.begin();
}

inline Rib::const_iterator
Rib::end() const
{
  return m_rib.end();
}

inline size_t
Rib::size() const
{
  return m_rib.size();
}

inline size_t
Rib::empty() const
{
  return m_rib.empty();
}

} // namespace nrd
} // namespace ndn

#endif // NRD_RIB_HPP
